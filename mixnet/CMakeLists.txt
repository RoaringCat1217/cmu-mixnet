add_executable(node node.c logger.c utils.c)
target_link_libraries(node
    fragment
)
