#include "shared/protocol/mappers/MoveRequestMapper.hpp"

namespace MoveRequestMapper {

MoveRequest toDomain(const MoveDto& dto) {
    return MoveRequest{Position{dto.fromRow, dto.fromCol}, Position{dto.toRow, dto.toCol}};
}

MoveDto toDto(const MoveRequest& request) {
    return MoveDto{request.from.row, request.from.col, request.to.row, request.to.col};
}

}  // namespace MoveRequestMapper
