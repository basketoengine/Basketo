#include "DevModeInputHandler.h"
#include "DevModeScene.h"
#include "imgui.h"
#include "../../vendor/imgui/backends/imgui_impl_sdl2.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/ColliderComponent.h" 
#include "../utils/Console.h"

static void closestPointOnSegment(float pX, float pY, float s1X, float s1Y, float s2X, float s2Y, float& cX, float& cY) {
    float segVX = s2X - s1X;
    float segVY = s2Y - s1Y;
    float t = ((pX - s1X) * segVX + (pY - s1Y) * segVY) / (segVX * segVX + segVY * segVY + 1e-6f); 
    t = std::max(0.0f, std::min(1.0f, t)); 
    cX = s1X + t * segVX;
    cY = s1Y + t * segVY;
}

std::tuple<int, int, float, float, float> getClosestEdgeToPoint(
    const std::vector<Vec2D>& vertices,
    float pointX, float pointY,
    const TransformComponent& transform,
    const ColliderComponent& collider,
    float cameraZoom
) {

    if (vertices.size() < 2) {
        return std::make_tuple(-1, -1, FLT_MAX, 0.0f, 0.0f);
    }

    float minDistanceSq = FLT_MAX;
    int closestEdgeA = -1;
    int closestEdgeB = -1;
    float closestEdgePointX = 0.0f;
    float closestEdgePointY = 0.0f;

    for (size_t i = 0; i < vertices.size(); ++i) {
        size_t j = (i + 1) % vertices.size();

        float v1x = transform.x + collider.offsetX + vertices[i].x;
        float v1y = transform.y + collider.offsetY + vertices[i].y;
        float v2x = transform.x + collider.offsetX + vertices[j].x;
        float v2y = transform.y + collider.offsetY + vertices[j].y;

        float cX, cY;
        closestPointOnSegment(pointX, pointY, v1x, v1y, v2x, v2y, cX, cY);

        float dx = pointX - cX;
        float dy = pointY - cY;
        float distanceSq = dx * dx + dy * dy;

        if (distanceSq < minDistanceSq) {
            minDistanceSq = distanceSq;
            closestEdgeA = static_cast<int>(i);
            closestEdgeB = static_cast<int>(j);
            closestEdgePointX = cX;
            closestEdgePointY = cY;
        }
    }
    return std::make_tuple(closestEdgeA, closestEdgeB, std::sqrt(minDistanceSq), closestEdgePointX, closestEdgePointY);
}


void handleDevModeInput(DevModeScene& scene, SDL_Event& event) {
    ImGuiIO& io = ImGui::GetIO();

    int rawMouseX, rawMouseY;
    SDL_GetMouseState(&rawMouseX, &rawMouseY);

    SDL_Rect& gameViewport = scene.gameViewport; 
    float& cameraX = scene.cameraX;
    float& cameraY = scene.cameraY;
    float& cameraZoom = scene.cameraZoom;
    bool& isPlaying = scene.isPlaying;
    bool& isDragging = scene.isDragging;
    bool& isResizing = scene.isResizing;
    ResizeHandle& activeHandle = scene.activeHandle;
    Entity& selectedEntity = scene.selectedEntity;
    ComponentManager* componentManager = scene.componentManager.get();
    EntityManager* entityManager = scene.entityManager.get();
    float& dragStartMouseX = scene.dragStartMouseX;
    float& dragStartMouseY = scene.dragStartMouseY;
    float& dragStartEntityX = scene.dragStartEntityX;
    float& dragStartEntityY = scene.dragStartEntityY;
    float& dragStartWidth = scene.dragStartWidth;
    float& dragStartHeight = scene.dragStartHeight;
    bool& snapToGrid = scene.snapToGrid;
    float& gridSize = scene.gridSize;
    char* inspectorTextureIdBuffer = scene.inspectorTextureIdBuffer;
    char* inspectorScriptPathBuffer = scene.inspectorScriptPathBuffer;

    bool mouseInViewport = (rawMouseX >= gameViewport.x && rawMouseX < (gameViewport.x + gameViewport.w) &&
                            rawMouseY >= gameViewport.y && rawMouseY < (gameViewport.y + gameViewport.h));

    float viewportMouseX = static_cast<float>(rawMouseX - gameViewport.x);
    float viewportMouseY = static_cast<float>(rawMouseY - gameViewport.y);

    float worldMouseX = cameraX + (viewportMouseX / cameraZoom);
    float worldMouseY = cameraY + (viewportMouseY / cameraZoom);

    static bool isPanning = false;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE && mouseInViewport && !io.WantCaptureMouse) {
        isPanning = true;
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_MIDDLE) {
        if (isPanning) {
            isPanning = false;
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
    }
    if (event.type == SDL_MOUSEMOTION && isPanning) {
        cameraX -= event.motion.xrel / cameraZoom; 
        cameraY -= event.motion.yrel / cameraZoom;
    }

    if (event.type == SDL_MOUSEWHEEL) {
        if (mouseInViewport && !io.WantCaptureMouse) {
            float oldCameraZoom = cameraZoom;
            float prevZoomEventWorldMouseX = cameraX + (viewportMouseX / oldCameraZoom); 
            float prevZoomEventWorldMouseY = cameraY + (viewportMouseY / oldCameraZoom);

            if (event.wheel.y > 0) cameraZoom *= 1.1f;
            if (event.wheel.y < 0) cameraZoom /= 1.1f;
            cameraZoom = std::clamp(cameraZoom, 0.1f, 10.0f); 

            if (cameraZoom != oldCameraZoom) {
                cameraX = prevZoomEventWorldMouseX - (viewportMouseX / cameraZoom);
                cameraY = prevZoomEventWorldMouseY - (viewportMouseY / cameraZoom);
            }
        }
    }

    if (isPlaying || isPanning) {
        isDragging = false;
        isResizing = false;
        activeHandle = ResizeHandle::NONE;
        scene.isEditingCollider = false; 
        scene.editingVertexIndex = -1;
        scene.isDraggingVertex = false;
        return;
    }
    
    if (scene.isEditingCollider && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<ColliderComponent>(selectedEntity) && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
        auto& collider = componentManager->getComponent<ColliderComponent>(selectedEntity);
        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
        static int hoveredEdgeA = -1, hoveredEdgeB = -1;
        static float hoveredEdgeX = 0, hoveredEdgeY = 0;
        static bool hoveringEdge = false;
        hoveringEdge = false;

        if (collider.vertices.size() >= 2) {
            auto [a, b, dist, cx, cy] = getClosestEdgeToPoint(collider.vertices, worldMouseX, worldMouseY, transform, collider, cameraZoom);
            if (dist < (12.0f / cameraZoom) ) { 
                hoveredEdgeA = a; hoveredEdgeB = b; hoveredEdgeX = cx; hoveredEdgeY = cy; hoveringEdge = true;
            }
        }

        if (mouseInViewport && !io.WantCaptureMouse) {
            switch (event.type) {
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        float minDistVertex = (8.0f / cameraZoom); 
                        minDistVertex *= minDistVertex; 
                        scene.editingVertexIndex = -1;
                        
                        for (size_t i = 0; i < collider.vertices.size(); ++i) {
                            float vx = transform.x + collider.offsetX + collider.vertices[i].x;
                            float vy = transform.y + collider.offsetY + collider.vertices[i].y;
                            float dx = worldMouseX - vx;
                            float dy = worldMouseY - vy;
                            if (dx * dx + dy * dy < minDistVertex) {
                                scene.editingVertexIndex = (int)i;
                                scene.isDraggingVertex = true;
                                break;
                            }
                        }
                        if (!scene.isDraggingVertex && hoveringEdge && hoveredEdgeA != -1 && hoveredEdgeB != -1) {
                            Vec2D newVert;
                            newVert.x = hoveredEdgeX - (transform.x + collider.offsetX);
                            newVert.y = hoveredEdgeY - (transform.y + collider.offsetY);
                            int insert_idx = (hoveredEdgeA + 1) % collider.vertices.size();
                            if (hoveredEdgeA == -1 && collider.vertices.empty()) { 
                                insert_idx = 0;
                            }
                            collider.vertices.insert(collider.vertices.begin() + insert_idx, newVert);
                            scene.editingVertexIndex = insert_idx; 
                            scene.isDraggingVertex = true; 
                        } else if (!scene.isDraggingVertex) { 
                            if (!hoveringEdge) {
                                Vec2D newVert;
                                newVert.x = worldMouseX - (transform.x + collider.offsetX);
                                newVert.y = worldMouseY - (transform.y + collider.offsetY);
                                collider.vertices.push_back(newVert);
                                scene.editingVertexIndex = collider.vertices.size() - 1;
                                scene.isDraggingVertex = true;
                            }
                        }
                    } else if (event.button.button == SDL_BUTTON_RIGHT) { 
                         float minDistVertex = (8.0f / cameraZoom);
                         minDistVertex *= minDistVertex;
                         int vertexToRemove = -1;
                         for (size_t i = 0; i < collider.vertices.size(); ++i) {
                            float vx = transform.x + collider.offsetX + collider.vertices[i].x;
                            float vy = transform.y + collider.offsetY + collider.vertices[i].y;
                            float dx = worldMouseX - vx;
                            float dy = worldMouseY - vy;
                            if (dx * dx + dy * dy < minDistVertex) {
                                vertexToRemove = (int)i;
                                break;
                            }
                        }
                        if (vertexToRemove != -1 && collider.vertices.size() > 0) { 
                            collider.vertices.erase(collider.vertices.begin() + vertexToRemove);
                            scene.isDraggingVertex = false; 
                            scene.editingVertexIndex = -1;
                        }
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        scene.isDraggingVertex = false;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (scene.isDraggingVertex && scene.editingVertexIndex >= 0 && scene.editingVertexIndex < collider.vertices.size()) {
                        collider.vertices[scene.editingVertexIndex].x = worldMouseX - (transform.x + collider.offsetX);
                        collider.vertices[scene.editingVertexIndex].y = worldMouseY - (transform.y + collider.offsetY);
                    }
                    break;
            }
        }
        isDragging = false;
        isResizing = false;
        activeHandle = ResizeHandle::NONE;
        return; 
    }


    if (!io.WantCaptureMouse && mouseInViewport) {
        switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    ResizeHandle handleClicked = ResizeHandle::NONE;
                    if (selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                        
                        handleClicked = scene.getHandleAtPosition(worldMouseX, worldMouseY, componentManager->getComponent<TransformComponent>(selectedEntity));
                    }

                    if (handleClicked != ResizeHandle::NONE) {
                        isResizing = true;
                        isDragging = false; 
                        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                        dragStartMouseX = worldMouseX;
                        dragStartMouseY = worldMouseY;
                        dragStartEntityX = transform.x;
                        dragStartEntityY = transform.y;
                        dragStartWidth = transform.width;
                        dragStartHeight = transform.height;
                        activeHandle = handleClicked;
                    } else {
                        isResizing = false; 
                        activeHandle = ResizeHandle::NONE;

                        std::vector<Entity> potentialSelections;
                        const auto& activeEntities = entityManager->getActiveEntities();
                        for (Entity entity : activeEntities) {
                            if (componentManager->hasComponent<TransformComponent>(entity)) {
                                if (scene.isMouseOverEntity(worldMouseX, worldMouseY, entity)) {
                                    potentialSelections.push_back(entity);
                                }
                            }
                        }

                        Entity topMostCandidate = NO_ENTITY_SELECTED;
                        if (!potentialSelections.empty()) {
                            if (potentialSelections.size() == 1) {
                                topMostCandidate = potentialSelections[0];
                            } else {
                                std::sort(potentialSelections.begin(), potentialSelections.end(),
                                    [&](Entity a, Entity b) {
                                        auto& transformA = componentManager->getComponent<TransformComponent>(a);
                                        auto& transformB = componentManager->getComponent<TransformComponent>(b);
                                        if (transformA.z_index != transformB.z_index) {
                                            return transformA.z_index > transformB.z_index;
                                        }
                                        float areaA = transformA.width * transformA.height;
                                        float areaB = transformB.width * transformB.height;
                                        if (std::abs(areaA - areaB) > 1e-6f) { 
                                            return areaA < areaB;
                                        }
                                        return a < b;
                                    }
                                );
                                topMostCandidate = potentialSelections[0];
                            }
                        }

                        if (topMostCandidate != NO_ENTITY_SELECTED) {
                            if (selectedEntity != topMostCandidate) {
                                selectedEntity = topMostCandidate;
                                inspectorTextureIdBuffer[0] = '\0'; 
                                inspectorScriptPathBuffer[0] = '\0';
                            }
                            isDragging = true;
                            auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                            dragStartMouseX = worldMouseX;
                            dragStartMouseY = worldMouseY;
                            dragStartEntityX = transform.x;
                            dragStartEntityY = transform.y;
                        } else {
                            selectedEntity = NO_ENTITY_SELECTED;
                            inspectorTextureIdBuffer[0] = '\0';
                            inspectorScriptPathBuffer[0] = '\0';
                            isDragging = false; 
                        }
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (isDragging && snapToGrid && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                        transform.x = std::roundf(transform.x / gridSize) * gridSize;
                        transform.y = std::roundf(transform.y / gridSize) * gridSize;
                    }
                    if (isResizing && snapToGrid && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                        
                        float snappedWidth = std::max(gridSize, std::roundf(transform.width / gridSize) * gridSize);
                        float snappedHeight = std::max(gridSize, std::roundf(transform.height / gridSize) * gridSize);
                        float snappedX = transform.x;
                        float snappedY = transform.y;

                        if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::BOTTOM_LEFT) {
                            snappedX = std::roundf((dragStartEntityX + dragStartWidth - snappedWidth) / gridSize) * gridSize;
                        } else { 
                            snappedX = std::roundf(dragStartEntityX / gridSize) * gridSize;
                        }
                        if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::TOP_RIGHT) {
                            snappedY = std::roundf((dragStartEntityY + dragStartHeight - snappedHeight) / gridSize) * gridSize;
                        } else { 
                            snappedY = std::roundf(dragStartEntityY / gridSize) * gridSize;
                        }
                        
                        transform.x = snappedX;
                        transform.y = snappedY;
                        transform.width = snappedWidth;
                        transform.height = snappedHeight;
                    }
                    isDragging = false;
                    isResizing = false;
                    activeHandle = ResizeHandle::NONE;
                }
                break;

            case SDL_MOUSEMOTION:
                if (isDragging && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                    float deltaX = worldMouseX - dragStartMouseX;
                    float deltaY = worldMouseY - dragStartMouseY;
                    auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                    transform.x = dragStartEntityX + deltaX;
                    transform.y = dragStartEntityY + deltaY;

                } else if (isResizing && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                    auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                    float deltaX = worldMouseX - dragStartMouseX;
                    float deltaY = worldMouseY - dragStartMouseY;

                    float newX = dragStartEntityX;
                    float newY = dragStartEntityY;
                    float newWidth = dragStartWidth;
                    float newHeight = dragStartHeight;

                    switch (activeHandle) {
                        case ResizeHandle::TOP_LEFT:
                            newX = dragStartEntityX + deltaX;
                            newY = dragStartEntityY + deltaY;
                            newWidth = dragStartWidth - deltaX;
                            newHeight = dragStartHeight - deltaY;
                            break;
                        case ResizeHandle::TOP_RIGHT:
                            newY = dragStartEntityY + deltaY;
                            newWidth = dragStartWidth + deltaX;
                            newHeight = dragStartHeight - deltaY;
                            break;
                        case ResizeHandle::BOTTOM_LEFT:
                            newX = dragStartEntityX + deltaX;
                            newWidth = dragStartWidth - deltaX;
                            newHeight = dragStartHeight + deltaY;
                            break;
                        case ResizeHandle::BOTTOM_RIGHT:
                            newWidth = dragStartWidth + deltaX;
                            newHeight = dragStartHeight + deltaY;
                            break;
                        case ResizeHandle::NONE:
                            break;
                    }

                    if (newWidth < HANDLE_SIZE) {
                        if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::BOTTOM_LEFT) {
                            newX = dragStartEntityX + dragStartWidth - HANDLE_SIZE;
                        }
                        newWidth = HANDLE_SIZE;
                    }
                    if (newHeight < HANDLE_SIZE) {
                        if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::TOP_RIGHT) {
                            newY = dragStartEntityY + dragStartHeight - HANDLE_SIZE;
                        }
                        newHeight = HANDLE_SIZE;
                    }

                    transform.x = newX;
                    transform.y = newY;
                    transform.width = newWidth;
                    transform.height = newHeight;
                }
                break;
        }
    } else {
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            isDragging = false;
            isResizing = false;
            activeHandle = ResizeHandle::NONE;
            scene.isDraggingVertex = false; 
        }
        if ((isDragging || isResizing || scene.isDraggingVertex) && event.type == SDL_MOUSEMOTION && !mouseInViewport) {

            isDragging = false;
            isResizing = false;
            activeHandle = ResizeHandle::NONE;
            scene.isDraggingVertex = false;
        }
    }
}

DevModeInputHandler::DevModeInputHandler(DevModeScene& scene)
    : m_sceneRef(scene) {}
