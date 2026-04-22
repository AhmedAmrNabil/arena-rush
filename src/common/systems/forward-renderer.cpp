#include "forward-renderer.hpp"

#include "../components/model-renderer.hpp"
#include "../components/post-process-effects.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"

namespace our {

    void ForwardRenderer::resizePostprocess(glm::ivec2 size) {
        if (!postprocessMaterial) return;

        glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);

        colorTarget->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTarget->getOpenGLName(), 0);

        depthTarget->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
                     nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTarget->getOpenGLName(), 0);

        Texture2D::unbind();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json& config) {
        // First, we store the window size for later use
        this->windowSize = windowSize;

        // Then we check if there is a sky texture in the configuration
        if (config.contains("sky")) {
            // First, we create a sphere which will be used to draw the sky
            this->skySphere = mesh_utils::sphere(glm::ivec2(16, 16));

            // We can draw the sky using the same shader used to draw textured objects
            ShaderProgram* skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
            skyShader->link();

            PipelineState skyPipelineState{};
            skyPipelineState.depthTesting.enabled = true;
            skyPipelineState.depthTesting.function = GL_LEQUAL;
            skyPipelineState.faceCulling.enabled = true;
            // cull front as we are looking at the sphere from the inside
            skyPipelineState.faceCulling.culledFace = GL_FRONT;
            skyPipelineState.faceCulling.frontFace = GL_CCW;

            // Load the sky texture (note that we don't need mipmaps since we want to avoid any unnecessary blurring
            // while rendering the sky)
            std::string skyTextureFile = config.value<std::string>("sky", "");
            Texture2D* skyTexture = texture_utils::loadImage(skyTextureFile, false);

            // Setup a sampler for the sky
            Sampler* skySampler = new Sampler();
            skySampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_WRAP_S, GL_REPEAT);
            skySampler->set(GL_TEXTURE_WRAP_T, GL_REPEAT);

            // Combine all the aforementioned objects (except the mesh) into a material
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = skySampler;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 0.0f;
            this->skyMaterial->transparent = false;
        }

        // Then we check if there is a postprocessing shader in the configuration
        if (config.contains("postprocess")) {
            glGenFramebuffers(1, &postprocessFrameBuffer);
            colorTarget = new Texture2D();
            depthTarget = new Texture2D();

            // Create a vertex array to use for drawing the texture
            glGenVertexArrays(1, &postProcessVertexArray);

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler* postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram* postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""), GL_FRAGMENT_SHADER);
            postprocessShader->link();

            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            postprocessMaterial->pipelineState.depthMask = false;

            resizePostprocess(windowSize);
        }
    }

    void ForwardRenderer::destroy() {
        // Delete all objects related to the sky
        if (skyMaterial) {
            delete skySphere;
            delete skyMaterial->shader;
            delete skyMaterial->texture;
            delete skyMaterial->sampler;
            delete skyMaterial;
            skySphere = nullptr;
            skyMaterial = nullptr;
        }
        // Delete all objects related to post processing
        if (postprocessMaterial) {
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
            colorTarget = nullptr;
            depthTarget = nullptr;
            postprocessMaterial = nullptr;
            postprocessFrameBuffer = 0;
            postProcessVertexArray = 0;
        }
    }

    void ForwardRenderer::render(World* world, glm::ivec2 windowSize) {
        if (windowSize.x <= 0 || windowSize.y <= 0) return;
        if (this->windowSize != windowSize) {
            this->windowSize = windowSize;
            resizePostprocess(windowSize);
        }

        // First of all, we search for a camera and for all the mesh renderers
        CameraComponent* camera = nullptr;
        opaqueCommands.clear();
        transparentCommands.clear();
        sceneLights.clear();
        weaponCommands.clear();
        for (auto entity : world->getEntities()) {
            // If we hadn't found a camera yet, we look for a camera in this entity
            if (!camera) camera = entity->getComponent<CameraComponent>();
            // If this entity has a mesh renderer component
            if (auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer) {
                // We construct a command from it
                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;
                if (command.material->transparent) {
                    transparentCommands.push_back(command);
                } else {
                    // Otherwise, we add it to the opaque command list
                    opaqueCommands.push_back(command);
                }
            }

            if (auto light = entity->getComponent<our::Light>(); light) {
                LightRenderData lightData;
                lightData.type = light->type;
                lightData.color = light->color;
                // compute world space position and direction of the light using the entity's transform
                glm::mat4 localToWorld = entity->getLocalToWorldMatrix();
                lightData.position = glm::vec3(localToWorld * glm::vec4(0, 0, 0, 1));
                // assume default light direction points to - z direction in local space
                // so away from the camera if the light is attached to the camera
                // this could be useful for rifle flashlight for example
                lightData.direction = glm::normalize(glm::vec3(localToWorld * glm::vec4(0, 0, -1, 0)));
                lightData.attenuation = light->attenuation;
                lightData.spotAngles = light->spotAngles;
                sceneLights.push_back(lightData);
            }

            if (auto modelRenderer = entity->getComponent<ModelRendererComponent>(); modelRenderer) {
                glm::mat4 modelMatrix = modelRenderer->getOwner()->getLocalToWorldMatrix();
                for (MeshRendererComponent* submesh : modelRenderer->model->getSubmeshes()) {
                    RenderCommand command;
                    command.localToWorld = modelMatrix * submesh->transform;
                    command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                    command.mesh = submesh->mesh;
                    command.material = submesh->material;

                    if (modelRenderer->firstPersonOverlay) {
                        weaponCommands.push_back(command);
                    } else if (command.material->transparent) {
                        transparentCommands.push_back(command);
                    } else {
                        opaqueCommands.push_back(command);
                    }
                }
            }
        }

        // If there is no camera, we return (we cannot render without a camera)
        if (camera == nullptr) return;

        glm::vec3 cameraForward =
            glm::normalize(glm::vec3(camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, -1, 0)));
        std::sort(transparentCommands.begin(), transparentCommands.end(),
                  [cameraForward](const RenderCommand& first, const RenderCommand& second) {
                      return glm::dot(first.center, cameraForward) > glm::dot(second.center, cameraForward);
                  });

        glm::vec3 cameraPosition = camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1);

        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

        glViewport(0, 0, windowSize.x, windowSize.y);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);

        // to ensure framebuffer is cleaned before rendering
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        // If there is a postprocess material, bind the framebuffer
        if (postprocessMaterial) {
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw all opaque objects first
        for (const RenderCommand& command : opaqueCommands) {
            command.material->setup();
            glm::mat4 MVP = VP * command.localToWorld;
            command.material->shader->set("transform", MVP);
            if (LitMaterial* litMaterial = dynamic_cast<LitMaterial*>(command.material); litMaterial) {
                // if the material is a lit material, we need to set the light uniforms
                litMaterial->setLightUniforms(sceneLights);
                litMaterial->shader->set("cameraPos", cameraPosition);
                litMaterial->shader->set("model", command.localToWorld);
            }
            command.mesh->draw();
        }

        // If there is a sky material, draw the sky
        if (this->skyMaterial) {
            skyMaterial->setup();

            glm::vec3 cameraPosition = camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cameraPosition);

            // clang-format off
            glm::mat4 alwaysBehindTransform = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 1.0f
            );
            // clang-format on

            skyMaterial->shader->set("transform", alwaysBehindTransform * VP * model);
            skySphere->draw();
        }

        // draw all the transparent commands
        for (const RenderCommand& command : transparentCommands) {
            command.material->setup();
            glm::mat4 MVP = VP * command.localToWorld;
            command.material->shader->set("transform", MVP);
            if (LitMaterial* litMaterial = dynamic_cast<LitMaterial*>(command.material); litMaterial) {
                // if the material is a lit material, we need to set the light uniforms
                litMaterial->setLightUniforms(sceneLights);
                litMaterial->shader->set("cameraPos", cameraPosition);
                litMaterial->shader->set("model", command.localToWorld);
            }
            command.mesh->draw();
        }

        // render weapon at the end with depth testing cleared
        // enable depth mask again so it actually get cleared
        glDepthMask(GL_TRUE);
        glClear(GL_DEPTH_BUFFER_BIT);

        // render weapon
        for (const RenderCommand& command : weaponCommands) {
            command.material->setup();
            glm::mat4 MVP = VP * command.localToWorld;
            command.material->shader->set("transform", MVP);
            if (LitMaterial* litMaterial = dynamic_cast<LitMaterial*>(command.material); litMaterial) {
                // if the material is a lit material, we need to set the light uniforms
                litMaterial->setLightUniforms(sceneLights);
                litMaterial->shader->set("cameraPos", cameraPosition);
                litMaterial->shader->set("model", command.localToWorld);
            }
            command.mesh->draw();
        }

        // If there is a postprocess material, apply postprocessing
        if (postprocessMaterial) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            postprocessMaterial->setup();

            for (auto entity : world->getEntities()) {
                if (auto* effects = entity->getComponent<our::PostProcessEffectsComponent>()) {
                    for (auto& [name, value] : effects->uniforms) {
                        postprocessMaterial->shader->set(name, value);
                    }
                    break;  // handles gameplay driven postprocess effects, other ones (like bloom) should be handled
                            // separately outside this loop
                }
            }

            glBindVertexArray(postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

}  // namespace our
