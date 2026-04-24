#include "forward-renderer.hpp"

#include "../components/model-renderer.hpp"
#include "../components/post-process-effects.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"
#include "components/animation.hpp"
#include "components/weapon.hpp"

namespace our {

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
            skyShader->attach("assets/shaders/textured-gamma.frag", GL_FRAGMENT_SHADER);
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
            Texture2D* skyTexture = texture_utils::loadImage(skyTextureFile, true);

            // Setup a sampler for the sky

            // Combine all the aforementioned objects (except the mesh) into a material
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = AssetLoader<ShaderProgram>::get("textured");
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = AssetLoader<Sampler>::get("default");
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 0.0f;
            this->skyMaterial->transparent = false;
        }

        auto ensureFullscreenVertexArray = [this]() {
            if (postProcessVertexArray == 0) {
                glGenVertexArrays(1, &postProcessVertexArray);
            }
        };

        // Then we check if there is a bloom postprocess configuration
        if (config.contains("bloom") && config["bloom"].is_object()) {
            const auto& bloomConfig = config["bloom"];
            if (bloomConfig.value("enabled", true)) {
                ensureFullscreenVertexArray();
                bloom = new BloomPostProcess();
                bloom->initialize(windowSize, bloomConfig);
            }
        }

        // Then we check if there is a postprocessing shader in the configuration
        if (config.contains("postprocess")) {
            postprocessFrameBuffer = new Framebuffer();
            postprocessFrameBuffer->resize(windowSize, true);

            // Create a vertex array to use for drawing the texture
            ensureFullscreenVertexArray();

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler* postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram* postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config["postprocess"].value<std::string>("fs", ""), GL_FRAGMENT_SHADER);
            postprocessShader->link();

            for (auto& [uniformName, uniformValue] : config["postprocess"]["uniforms"].items()) {
                postprocessUniforms[uniformName] = uniformValue.get<float>();
            }

            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = postprocessFrameBuffer->getColorTarget();
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            postprocessMaterial->pipelineState.depthMask = false;
        }

        bonesUniformBuffer = new UniformBuffer(MAX_BONES * sizeof(glm::mat4), bonesBindingPoint);
    }

    void ForwardRenderer::destroy() {
        // Delete all objects related to the sky
        if (skyMaterial) {
            delete skySphere;
            delete skyMaterial->texture;
            delete skyMaterial;
            skySphere = nullptr;
            skyMaterial = nullptr;
        }
        // Delete all objects related to bloom
        if (bloom) {
            bloom->destroy();
            delete bloom;
            bloom = nullptr;
        }

        // Delete all objects related to post processing
        if (postprocessMaterial) {
            delete postprocessFrameBuffer;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
            postprocessMaterial = nullptr;
            postprocessFrameBuffer = nullptr;
        }

        if (postProcessVertexArray != 0) {
            glDeleteVertexArrays(1, &postProcessVertexArray);
            postProcessVertexArray = 0;
        }

        if (bonesUniformBuffer) {
            delete bonesUniformBuffer;
            bonesUniformBuffer = nullptr;
        }
    }

    void ForwardRenderer::render(World* world, glm::ivec2 windowSize) {
        if (windowSize.x <= 0 || windowSize.y <= 0) return;
        if (this->windowSize != windowSize) {
            this->windowSize = windowSize;
            if (postprocessFrameBuffer) postprocessFrameBuffer->resize(windowSize, true);
            if (bloom) bloom->resize(windowSize);
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
                if (auto anim = entity->getComponent<AnimationComponent>(); anim) {
                    if (meshRenderer->hasBones) {
                        command.animator = &anim->animator;
                    } else if (auto nodeTransform = anim->animator.getNodeTransform(meshRenderer->nodeName);
                               nodeTransform) {
                        command.localToWorld = command.localToWorld * (*nodeTransform);
                    }
                }
                // if it is transparent, we add it to the transparent commands list
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
                if (!modelRenderer->model) {
                    continue;  // model failed to load, skip rendering
                }

                if (gameplay::WeaponComponent* weapon = entity->getComponent<gameplay::WeaponComponent>(); weapon) {
                    if (!weapon->isActive) continue;  // if the weapon is not active, skip rendering
                }

                glm::mat4 modelMatrix = modelRenderer->getOwner()->getLocalToWorldMatrix();
                for (MeshRendererComponent* submesh : modelRenderer->model->getSubmeshes()) {
                    RenderCommand command;
                    command.localToWorld = modelMatrix * submesh->transform;
                    command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                    command.mesh = submesh->mesh;
                    command.material = submesh->material;
                    if (auto anim = entity->getComponent<AnimationComponent>(); anim) {
                        if (submesh->hasBones) {
                            command.animator = &anim->animator;
                        } else if (auto nodeTransform = anim->animator.getNodeTransform(submesh->nodeName);
                                   nodeTransform) {
                            command.localToWorld = modelMatrix * (*nodeTransform);
                        }
                    }

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

        // If there is a bloom or postprocess target, bind it before rendering the scene
        Framebuffer* sceneTarget = nullptr;
        if (bloom) {
            sceneTarget = bloom->getSceneFramebuffer();
        } else if (postprocessMaterial) {
            sceneTarget = postprocessFrameBuffer;
        }

        if (sceneTarget) {
            sceneTarget->bind();
        } else {
            Framebuffer::unbind();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Animator* lastUploadedAnimator = nullptr;
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
            if (command.animator) {
                command.material->shader->bindUniformBlock("Bones", bonesBindingPoint);
                // to optimize performance, we only update the bone matrices uniform buffer
                // if the animator is different from the last drawn command's animator
                if (command.animator != lastUploadedAnimator) {
                    const std::vector<glm::mat4>& boneMatrices = command.animator->getFinalBoneMatrices();
                    bonesUniformBuffer->bind();
                    bonesUniformBuffer->update(boneMatrices);
                    lastUploadedAnimator = command.animator;
                }
                command.material->shader->set("hasBones", 1);
            } else {
                command.material->shader->set("hasBones", 0);
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
            if (command.animator) {
                command.material->shader->bindUniformBlock("Bones", bonesBindingPoint);
                const std::vector<glm::mat4>& boneMatrices = command.animator->getFinalBoneMatrices();
                bonesUniformBuffer->bind();
                bonesUniformBuffer->update(boneMatrices.data(), sizeof(glm::mat4) * boneMatrices.size());
                command.material->shader->set("hasBones", 1);
            } else {
                command.material->shader->set("hasBones", 0);
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
            if (command.animator) {
                command.material->shader->bindUniformBlock("Bones", bonesBindingPoint);
                const std::vector<glm::mat4>& boneMatrices = command.animator->getFinalBoneMatrices();
                bonesUniformBuffer->bind();
                bonesUniformBuffer->update(boneMatrices.data(), sizeof(glm::mat4) * boneMatrices.size());
                command.material->shader->set("hasBones", 1);
            } else {
                command.material->shader->set("hasBones", 0);
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

        if (bloom) {
            Framebuffer* bloomOutput = postprocessMaterial ? postprocessFrameBuffer : nullptr;
            bloom->render(postProcessVertexArray, bloomOutput);
        }

        // If there is a postprocess material, apply postprocessing
        if (postprocessMaterial) {
            Framebuffer::unbind();
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

            for (auto& [uniformName, uniformValue] : postprocessUniforms) {
                postprocessMaterial->shader->set(uniformName, uniformValue);
            }

            glBindVertexArray(postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

}  // namespace our
