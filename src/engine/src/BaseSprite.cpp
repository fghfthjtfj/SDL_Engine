#include "BaseSprite.h"

BaseSprite::BaseSprite(const std::string& model_path)
    : model_path(model_path), position(glm::mat4(1.0f)), dirty(true)
{
}

BaseSprite::~BaseSprite() {}

void BaseSprite::SetPosition(const glm::vec3& position)
{
    this->position = glm::translate(glm::mat4(1.0f), position);
    dirty = true;
}

void BaseSprite::Move(const glm::vec3& delta)
{
    this->position = glm::translate(this->position, delta);
    dirty = true;
}

void BaseSprite::Rotate(float angle_deg, const glm::vec3& axis)
{
    this->position = glm::rotate(this->position, glm::radians(angle_deg), axis);
    dirty = true;
}
