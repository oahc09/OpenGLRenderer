#include <postprocess.h>
#include <render_texture.h>
#include <renderer_window.h>

PostProcess::PostProcess(RendererWindow window, Shader *_shader) : shader(_shader) 
{
    rt      = new RenderTexture(window.Width(), window.Height());
    width   = rt->width;
    height  = rt->height;
    InitPostProcess();
}

PostProcess::~PostProcess()
{
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteRenderbuffers(1, &rbo);
    delete rt;
}

void PostProcess::ResizeRenderArea(int x, int y)
{
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteRenderbuffers(1, &rbo);
    delete rt;

    rt      = new RenderTexture(x, y);
    width   = rt->width;
    height  = rt->height;
    CreateRenderBuffer();
    CreateFrameBuffer();
}

/*******************************************************************
* Create a frame buffer and bind RenderTexture and Render Buffer.
* Need to call after CreateRenderBuffer.
********************************************************************/
void PostProcess::CreateFrameBuffer()
{
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->color_buffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/******************************************
* Create a render buffer.
******************************************/
void PostProcess::CreateRenderBuffer()
{
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo); 
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);  
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void PostProcess::InitPostProcess()
{
    // Screen quad VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // Create frame buffer and render buffer
    CreateRenderBuffer();
    CreateFrameBuffer();

}

void PostProcess::DrawPostProcessResult()
{
    // now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // clear all relevant buffers
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
    glClear(GL_COLOR_BUFFER_BIT);

    shader->use();
    glBindVertexArray(quadVAO);
    glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
    glBindTexture(GL_TEXTURE_2D, rt->color_buffer);	// use the color attachment texture as the texture of the quad plane
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}