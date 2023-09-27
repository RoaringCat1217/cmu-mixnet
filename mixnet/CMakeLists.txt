add_executable(node node.c logger.c stp.c utils.c attr.c lsa.c ll.c routing.c)
target_link_libraries(node
    fragment
)
