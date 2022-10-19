#pragma once

#include "core/common.h"
#include "zen-remote/server/rendering-unit.h"

namespace zen::remote::server {

class Remote;

class RenderingUnit final : public IRenderingUnit {
 public:
  DISABLE_MOVE_AND_COPY(RenderingUnit);
  RenderingUnit() = delete;
  RenderingUnit(std::shared_ptr<Remote> remote);
  ~RenderingUnit();

  void Init();

 private:
  std::shared_ptr<Remote> remote_;
  uint64_t id_;
};

}  // namespace zen::remote::server
