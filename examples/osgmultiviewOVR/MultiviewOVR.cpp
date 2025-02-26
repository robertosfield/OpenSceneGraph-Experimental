#include "MultiviewOVR.h"

#include <osgViewer/Renderer>
#include <osgViewer/View>
#include <osgViewer/GraphicsWindow>

#include <osg/io_utils>

#include <osg/Texture2DArray>
#include <osg/TextureRectangle>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osg/Stencil>
#include <osg/PolygonStipple>
#include <osg/ValueObject>
#include <osg/DisplaySettings>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

using namespace osgViewer;


struct CustomIntialFrustumCallback : public osg::CullSettings::InitialFrustumCallback
{
    std::vector<osg::Matrixd> projectionOffsets;
    std::vector<osg::Matrixd> viewOffsets;

    bool applyBB = true;
    osg::BoundingBoxd bb;

    void toggle()
    {
        applyBB = !applyBB;
    }

    void computeClipSpaceBound(osg::Camera& camera)
    {
        osg::Matrixd pmv = camera.getProjectionMatrix() * camera.getViewMatrix();

        size_t numOffsets = std::min(projectionOffsets.size(), viewOffsets.size());

        std::vector<osg::Vec3d> world_vertices;
        world_vertices.reserve(numOffsets*8);

        for(size_t i=0; i<numOffsets; ++i)
        {
            osg::Matrixd proj = projectionOffsets[i] * camera.getProjectionMatrix();
            osg::Matrixd view = viewOffsets[i] * camera.getViewMatrix();

            osg::Matrix clipToWorld;
            clipToWorld.invert(proj * view);

            world_vertices.push_back(osg::Vec3d(-1.0, -1.0, -1.0) * clipToWorld);
            world_vertices.push_back(osg::Vec3d(1.0, -1.0, -1.0) * clipToWorld);
            world_vertices.push_back(osg::Vec3d(1.0, 1.0, -1.0) * clipToWorld);
            world_vertices.push_back(osg::Vec3d(-1.0, 1.0, -1.0) * clipToWorld);

            world_vertices.push_back(osg::Vec3d(-1.0, -1.0, 1.0) * clipToWorld);
            world_vertices.push_back(osg::Vec3d(1.0, -1.0, 1.0) * clipToWorld);
            world_vertices.push_back(osg::Vec3d(1.0, 1.0, 1.0) * clipToWorld);
            world_vertices.push_back(osg::Vec3d(-1.0, 1.0, 1.0) * clipToWorld);

            // project local clip space into world coords
            // project world coords back into master clipspace
        }

        osg::Matrix worldToclip = camera.getProjectionMatrix() * camera.getViewMatrix();

        for(auto& v : world_vertices)
        {
            bb.expandBy(v * worldToclip);
        }
    }

    virtual void setInitialFrustum(osg::CullStack& cullStack, osg::Polytope& frustum) const
    {
        osg::CullSettings::CullingMode cullingMode = cullStack.getCullingMode();
        if (applyBB)
        {
            frustum.setToBoundingBox(bb, ((cullingMode&osg::CullSettings::NEAR_PLANE_CULLING)!=0),((cullingMode&osg::CullSettings::FAR_PLANE_CULLING)!=0));
        }
        else
        {
            frustum.setToUnitFrustum(((cullingMode&osg::CullSettings::NEAR_PLANE_CULLING)!=0),((cullingMode&osg::CullSettings::FAR_PLANE_CULLING)!=0));
        }
    }
};

class ToggleFrustumHandler : public osgGA::GUIEventHandler
{
    public:

        ToggleFrustumHandler(CustomIntialFrustumCallback* callback) :
            cifc(callback) {}

        osg::ref_ptr<CustomIntialFrustumCallback> cifc;

        bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&)
        {
            if (ea.getHandled()) return false;

            switch(ea.getEventType())
            {
                case(osgGA::GUIEventAdapter::KEYUP):
                {
                    if (ea.getKey()=='c')
                    {
                        cifc->toggle();
                        return true;
                    }
                    break;
                }

                default:
                    return false;
            }
            return false;
        }
};


osg::ref_ptr<osg::Node> MultiviewOVR::createStereoMesh(const osg::Vec3& origin, const osg::Vec3& widthVector, const osg::Vec3& heightVector) const
{
    osg::Vec3d center(0.0,0.0,0.0);
    osg::Vec3d eye(0.0,0.0,0.0);

    // create the quad to visualize.
    osg::Geometry* geometry = new osg::Geometry();

    geometry->setSupportsDisplayList(false);

    osg::DrawElementsUShort* elements = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES);
    osg::Vec3Array* vertices = new osg::Vec3Array;
    osg::Vec3Array* texcoords = new osg::Vec3Array;
    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0, 1.0, 0.0, 1.0));

    // left hand side
    vertices->push_back(origin); texcoords->push_back(osg::Vec3(0.0f, 0.0f, 0.0f));
    vertices->push_back(origin + widthVector*0.5f); texcoords->push_back(osg::Vec3(1.0f, 0.0f, 0.0f));
    vertices->push_back(origin + widthVector*0.5f +heightVector); texcoords->push_back(osg::Vec3(1.0f, 1.0f, 0.0f));
    vertices->push_back(origin + heightVector); texcoords->push_back(osg::Vec3(0.0f, 1.0f, 0.0f));

    elements->push_back(0);
    elements->push_back(1);
    elements->push_back(2);
    elements->push_back(2);
    elements->push_back(3);
    elements->push_back(0);

    // right hand side
    vertices->push_back(origin + widthVector*0.5f); texcoords->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
    vertices->push_back(origin + widthVector); texcoords->push_back(osg::Vec3(1.0f, 0.0f, 1.0f));
    vertices->push_back(origin + widthVector +heightVector); texcoords->push_back(osg::Vec3(1.0f, 1.0f, 1.0f));
    vertices->push_back(origin + widthVector*0.5f + heightVector); texcoords->push_back(osg::Vec3(0.0f, 1.0f, 1.0f));

    elements->push_back(4);
    elements->push_back(5);
    elements->push_back(6);
    elements->push_back(6);
    elements->push_back(7);
    elements->push_back(4);

    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors, osg::Array::BIND_OVERALL);
    geometry->setTexCoordArray(0, texcoords);
    geometry->addPrimitiveSet(elements);

    return geometry;
}

void MultiviewOVR::configure(osgViewer::View& view) const
{
    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (!wsi)
    {
        OSG_NOTICE<<"Error, no WindowSystemInterface available, cannot create windows."<<std::endl;
        return;
    }

    osg::DisplaySettings* displaySettings = getActiveDisplaySetting(view);


    osg::GraphicsContext::ScreenIdentifier si;
    si.readDISPLAY();

    // displayNum has not been set so reset it to 0.
    if (si.displayNum<0) si.displayNum = 0;

    si.screenNum = _screenNum;

    unsigned int width, height;
    wsi->getScreenResolution(si, width, height);

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->hostName = si.hostName;
    traits->displayNum = si.displayNum;
    traits->screenNum = si.screenNum;
    traits->x = 0;
    traits->y = 0;
    traits->width = width;
    traits->height = height;
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (!gc)
    {
        OSG_NOTICE<<"GraphicsWindow has not been created successfully."<<std::endl;
        return;
    }

    int tex_width = width;
    int tex_height = height;

    int camera_width = tex_width;
    int camera_height = tex_height;

    osg::Texture2DArray* color_texture = new osg::Texture2DArray;

    color_texture->setTextureSize(tex_width, tex_height, 2);
    color_texture->setInternalFormat(GL_RGBA);
    color_texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
    color_texture->setFilter(osg::Texture::MAG_FILTER,osg::Texture::LINEAR);
    color_texture->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
    color_texture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
    color_texture->setWrap(osg::Texture::WRAP_R,osg::Texture::CLAMP_TO_EDGE);

    osg::Texture2DArray* depth_texture = new osg::Texture2DArray;

    depth_texture->setTextureSize(tex_width, tex_height, 2);
    depth_texture->setInternalFormat(GL_DEPTH_COMPONENT);
    depth_texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
    depth_texture->setFilter(osg::Texture::MAG_FILTER,osg::Texture::LINEAR);
    depth_texture->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
    depth_texture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
    depth_texture->setWrap(osg::Texture::WRAP_R,osg::Texture::CLAMP_TO_EDGE);

    osg::Camera::RenderTargetImplementation renderTargetImplementation = osg::Camera::FRAME_BUFFER_OBJECT;
    GLenum buffer = GL_FRONT;


    // left/right eye multiviewOVR camera
    {
        // GL_OVR_multiview2 extensions requires modern versions of GLSL without fixed function fallback
        gc->getState()->setUseModelViewAndProjectionUniforms(true);
        gc->getState()->setUseVertexAttributeAliasing(true);
        //osg::DisplaySettings::instance()->setShaderHint(osg::DisplaySettings::SHADER_GL3);

        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setName("multview eye camera");
        camera->setGraphicsContext(gc.get());
        camera->setViewport(new osg::Viewport(0,0,camera_width, camera_height));
        camera->setDrawBuffer(buffer);
        camera->setReadBuffer(buffer);
        camera->setAllowEventFocus(false);

        osg::ref_ptr<CustomIntialFrustumCallback> ifc = new CustomIntialFrustumCallback;


        view.addEventHandler(new ToggleFrustumHandler(ifc.get()));

        // assign custom frustum callback
        camera->setInitialFrustumCallback(ifc.get());


        // tell the camera to use OpenGL frame buffer object where supported.
        camera->setRenderTargetImplementation(renderTargetImplementation);

        // attach the texture and use it as the color buffer, specify that the face is controlled by the multiview extension
        camera->attach(osg::Camera::COLOR_BUFFER, color_texture, 0, osg::Camera::FACE_CONTROLLED_BY_MULTIVIEW_SHADER);
        camera->attach(osg::Camera::DEPTH_BUFFER, depth_texture, 0, osg::Camera::FACE_CONTROLLED_BY_MULTIVIEW_SHADER);


        view.addSlave(camera.get(), osg::Matrixd(), osg::Matrixd());

        osg::StateSet* stateset = camera->getOrCreateStateSet();
        {
            // set up the projection and view matrix uniforms
            ifc->projectionOffsets.push_back(displaySettings->computeLeftEyeProjectionImplementation(osg::Matrixd()));
            ifc->viewOffsets.push_back(displaySettings->computeLeftEyeViewImplementation(osg::Matrixd()));

            ifc->projectionOffsets.push_back(displaySettings->computeRightEyeProjectionImplementation(osg::Matrixd()));
            ifc->viewOffsets.push_back(displaySettings->computeRightEyeViewImplementation(osg::Matrixd()));

            ifc->computeClipSpaceBound(*camera);

            osg::ref_ptr<osg::Uniform> ovr_viewMatrix_uniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "ovr_viewMatrix", ifc->projectionOffsets.size());
            osg::ref_ptr<osg::Uniform> ovr_projectionMatrix_uniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "ovr_projectionMatrix", ifc->viewOffsets.size());
            stateset->addUniform(ovr_viewMatrix_uniform);
            stateset->addUniform(ovr_projectionMatrix_uniform);

            for(size_t i=0; i<ifc->projectionOffsets.size(); ++i)
            {
                ovr_projectionMatrix_uniform->setElement(i, ifc->projectionOffsets[i]);
            }

            for(size_t i=0; i<ifc->viewOffsets.size(); ++i)
            {
                ovr_viewMatrix_uniform->setElement(i, ifc->viewOffsets[i]);
            }

            // set up the shaders
            osg::ref_ptr<osg::Program> program = new osg::Program();
            stateset->setAttribute(program.get(), osg::StateAttribute::ON);

            std::string vsFileName("multiviewOVR.vert");
            std::string fsFileName("multiviewOVR.frag");

            osg::ref_ptr<osg::Shader> vertexShader = osgDB::readRefShaderFile( osg::Shader::VERTEX, vsFileName) ;
            if (vertexShader.get()) program->addShader( vertexShader.get() );

            osg::ref_ptr<osg::Shader> fragmentShader = osgDB::readRefShaderFile( osg::Shader::FRAGMENT, fsFileName) ;
            if (fragmentShader.get()) program->addShader( fragmentShader.get() );
        }

    }

    view.getCamera()->setProjectionMatrixAsPerspective(90.0f, 1.0, 1, 1000.0);

    // distortion correction set up.
    {
        osg::ref_ptr<osg::Node> mesh = createStereoMesh(osg::Vec3(0.0f,0.0f,0.0f), osg::Vec3(width,0.0f,0.0f), osg::Vec3(0.0f,height,0.0f));

        // new we need to add the texture to the mesh, we do so by creating a
        // StateSet to contain the Texture StateAttribute.
        osg::StateSet* stateset = mesh->getOrCreateStateSet();
        stateset->setTextureAttribute(0, color_texture, osg::StateAttribute::ON);
        stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        {
            osg::ref_ptr<osg::Program> program = new osg::Program();
            stateset->setAttribute(program.get(), osg::StateAttribute::ON);

            std::string vsFileName("standard.vert");
            std::string fsFileName("standard.frag");

            osg::ref_ptr<osg::Shader> vertexShader = osgDB::readRefShaderFile( osg::Shader::VERTEX, vsFileName) ;
            if (vertexShader.get()) program->addShader( vertexShader.get() );

            osg::ref_ptr<osg::Shader> fragmentShader = osgDB::readRefShaderFile( osg::Shader::FRAGMENT, fsFileName) ;
            if (fragmentShader.get()) program->addShader( fragmentShader.get() );
        }

        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setGraphicsContext(gc.get());
        camera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
        camera->setClearColor( osg::Vec4(0.0,0.0,0.0,1.0) );
        camera->setViewport(new osg::Viewport(0, 0, width, height));

        GLenum window_buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
        camera->setDrawBuffer(window_buffer);
        camera->setReadBuffer(window_buffer);
        camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        camera->setAllowEventFocus(true);
        camera->setInheritanceMask(camera->getInheritanceMask() & ~osg::CullSettings::CLEAR_COLOR & ~osg::CullSettings::COMPUTE_NEAR_FAR_MODE);
        //camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

        camera->setProjectionMatrixAsOrtho2D(0,width,0,height);
        camera->setViewMatrix(osg::Matrix::identity());

        // add subgraph to render
        camera->addChild(mesh.get());

        camera->setName("DistortionCorrectionCamera");

        osgDB::writeNodeFile(*mesh, "mesh.osgt");

        view.addSlave(camera.get(), osg::Matrixd(), osg::Matrixd(), false);
    }

    view.getCamera()->setNearFarRatio(0.0001f);

    if (view.getLightingMode()==osg::View::HEADLIGHT)
    {
        // set a local light source for headlight to ensure that lighting is consistent across sides of cube.
        view.getLight()->setPosition(osg::Vec4(0.0f,0.0f,0.0f,1.0f));
    }
}
