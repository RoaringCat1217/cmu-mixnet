add_executable(node node.c logger.c)
target_link_libraries(node
    fragment
)
