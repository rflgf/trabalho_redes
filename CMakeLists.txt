cmake_minimum_required(VERSION 3.13)

project(trabalho_redes)

set(CMAKE_C_FLAGS "-Wall")

set(trabalho_redes_src
	"src/main.c"
	"src/packet.c"
	"src/packet_handler.c"
	"src/receiver.c"
	"src/router.c"
	"src/sender.c"
	"src/table_handler.c"
	"src/utils.c"
)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(trabalho_redes ${trabalho_redes_src})

target_include_directories(trabalho_redes PUBLIC "src")

target_link_libraries(trabalho_redes Threads::Threads)
