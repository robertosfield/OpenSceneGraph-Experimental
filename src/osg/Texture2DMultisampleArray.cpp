/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2010 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
 *
*/

#include <osg/GLExtensions>
#include <osg/Texture2DMultisampleArray>
#include <osg/State>
#include <osg/Notify>

using namespace osg;

Texture2DMultisampleArray::Texture2DMultisampleArray():
            _textureWidth(0),
            _textureHeight(0),
            _textureDepth(0),
            _numSamples(1),
            _fixedsamplelocations(GL_FALSE)
{
}

Texture2DMultisampleArray::Texture2DMultisampleArray(GLsizei numSamples, GLboolean fixedsamplelocations):
            _textureWidth(0),
            _textureHeight(0),
            _textureDepth(0),
            _numSamples(numSamples),
            _fixedsamplelocations(fixedsamplelocations)
{
}

Texture2DMultisampleArray::Texture2DMultisampleArray(int width, int height, int depth, GLenum internalFormat, GLsizei numSamples, GLboolean fixedsamplelocations) :
    _textureWidth(width),
    _textureHeight(height),
    _textureDepth(depth),
    _numSamples(numSamples),
    _fixedsamplelocations(fixedsamplelocations)
{
    setInternalFormat(internalFormat);
}

Texture2DMultisampleArray::Texture2DMultisampleArray(const Texture2DMultisampleArray& text,const CopyOp& copyop):
            Texture(text,copyop),
            _textureWidth(text._textureWidth),
            _textureHeight(text._textureHeight),
            _textureDepth(text._textureDepth),
            _numSamples(text._numSamples),
            _fixedsamplelocations(text._fixedsamplelocations)
{
}

Texture2DMultisampleArray::~Texture2DMultisampleArray()
{
}

int Texture2DMultisampleArray::compare(const StateAttribute& sa) const
{
    // check the types are equal and then create the rhs variable
    // used by the COMPARE_StateAttribute_Parameter macros below.
    COMPARE_StateAttribute_Types(Texture2DMultisampleArray,sa)


    int result = compareTexture(rhs);
    if (result!=0) return result;

    // compare each parameter in turn against the rhs.
    if (_textureWidth != 0 && rhs._textureWidth != 0)
    {
        COMPARE_StateAttribute_Parameter(_textureWidth)
    }
    if (_textureHeight != 0 && rhs._textureHeight != 0)
    {
        COMPARE_StateAttribute_Parameter(_textureHeight)
    }
    if (_textureDepth != 0 && rhs._textureDepth != 0)
    {
        COMPARE_StateAttribute_Parameter(_textureDepth)
    }
    if (_numSamples != 0 && rhs._numSamples != 0)
    {
        COMPARE_StateAttribute_Parameter(_numSamples)
    }
    if (_fixedsamplelocations != 0 && rhs._fixedsamplelocations != 0)
    {
        COMPARE_StateAttribute_Parameter(_fixedsamplelocations)
    }


    return 0; // passed all the above comparison macros, must be equal.
}

void Texture2DMultisampleArray::apply(State& state) const
{
    // current OpenGL context.
    const unsigned int contextID = state.getContextID();
    const GLExtensions* extensions = state.get<GLExtensions>();
    if (!extensions->isTextureMultisampledSupported)
    {
        OSG_INFO<<"Texture2DMultisampleArray not supported."<<std::endl;
        return;
    }

#if 0
    Texture::TextureObjectManager* tom = Texture::getTextureObjectManager(contextID).get();
    ElapsedTime elapsedTime(&(tom->getApplyTime()));
    tom->getNumberApplied()++;
#endif

    // get the texture object for the current contextID.
    TextureObject* textureObject = getTextureObject(contextID);

    if (textureObject)
    {
        textureObject->bind();
    }
    else if ( (_textureWidth!=0) && (_textureHeight!=0) && (_textureDepth != 0) && (_numSamples!=0) )
    {
        textureObject = generateAndAssignTextureObject(
                                 contextID,
                                 getTextureTarget(),
                                 1,
                                 _internalFormat,
                                 _textureWidth,
                                 _textureHeight,
                                 _textureDepth,
                                 _borderWidth);

        textureObject->bind();

        extensions->glTexImage3DMultisample( getTextureTarget(),
                                             _numSamples,
                                             _internalFormat,
                                             _textureWidth,
                                             _textureHeight,
                                             _textureDepth,
                                             _fixedsamplelocations );
    }
    else
    {
        glBindTexture( getTextureTarget(), 0 );
    }
}

void Texture2DMultisampleArray::computeInternalFormat() const
{
    computeInternalFormatType();
}

