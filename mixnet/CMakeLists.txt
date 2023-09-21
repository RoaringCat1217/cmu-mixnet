add_executable(node node.c logger.c stp.c utils.c attr.c)
target_link_libraries(node
    fragment
)
