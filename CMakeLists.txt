cmake_minimum_required(VERSION 3.13)

project(trabalho_redes)

set(CMAKE_C_FLAGS "-Wall")

#file(GLOB trabalho_redes CONFIGURE_DEPENDS "*.h" "*.c")
set(trabalho_redes "main.c"
				   "router.c"
				   "package.c"
				   "utils.c"
)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(trabalho_redes ${trabalho_redes})

target_link_libraries(trabalho_redes Threads::Threads)
