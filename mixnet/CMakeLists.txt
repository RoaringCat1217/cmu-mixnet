add_executable(node node.c logger.c stp.c util.c attr.c)
target_link_libraries(node
    fragment
)
