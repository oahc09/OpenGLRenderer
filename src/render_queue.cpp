#include <render_queue.h>
#include <scene_object.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <camera.h>
#include <shader.h>
#include <renderer_window.h>
#include <postprocess.h>
#include <render_texture.h>
#include <editor_settings.h>
#include <model.h>
#include <shader.h>

void RenderQueue::EnqueueRenderQueue(SceneModel *model)     { ModelQueueForRender.insert({model->id, model});   }
void RenderQueue::RemoveFromRenderQueue(unsigned int id)    { ModelQueueForRender.erase(id);                    }

SceneModel *RenderQueue::GetRenderModel(unsigned int id)
{
    if (ModelQueueForRender.find(id) != ModelQueueForRender.end())
    {
        return ModelQueueForRender[id];
    }
    else
    {
        return nullptr;
    }
}

/****************************************************************
* Render order is decide by create order by now.
* If there's a requirement to render model with alpha
* we need to sort models by distance to camera. 
* All models is rendered without alpha clip or alpha blend now.
*****************************************************************/
void RenderQueue::Render(RendererWindow *window, Camera *camera)
{
    // Pre Render Setting
    if (EditorSettings::UsePostProcess && window->postprocess != nullptr)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, window->postprocess->framebuffer);
    }
    glClearColor(clear_color[0],clear_color[1],clear_color[2],1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (EditorSettings::UsePolygonMode)
    {
        glLineWidth(0.05);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)window->Width() / (float)window->Height(), 0.1f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();


    // Render Scene
    for (std::map<unsigned int, SceneModel *>::iterator it = ModelQueueForRender.begin(); it != ModelQueueForRender.end(); it++)
    {
        // a SceneModel's meshes share one transform
        // so we will set the model matrix only once while render a SceneModel
        SceneModel *sm = it->second;
        Material* mat = sm->meshRenderers[0]->material;
        Shader* shader;
        if (EditorSettings::UsePolygonMode)
        {
            shader = Shader::LoadedShaders["default.fs"];
        }
        else
        {
            shader = mat->shader;
        }
        shader->use();
        shader->setMat4("projection", projection);
        shader->setMat4("view", view);
        shader->setVec3("viewPos", camera->Position);

        // Render the loaded model
        ATR_Transform *transform = sm->transform;
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, transform->Position); // translate it down so it's at the center of the scene
        m = glm::rotate(m, glm::radians(transform->Rotation.r), glm::vec3(1, 0, 0));
        m = glm::rotate(m, glm::radians(transform->Rotation.g), glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(transform->Rotation.b), glm::vec3(0, 0, 1));
        m = glm::scale(m, transform->Scale); // it's a bit too big for our scene, so scale it down
        shader->setMat4("model", m);

        sm->DrawAllMeshRenderer();
    }

    // PostProcess
    if (EditorSettings::UsePostProcess && window->postprocess != nullptr)
    {
        window->postprocess->DrawPostProcessResult();
    }
}