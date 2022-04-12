cmake_minimum_required(VERSION 3.14)

project(mnser)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11") 
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11")
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include (cmake/utils.cmake)

include_directories(.)
include_directories(inc)
include_directories(inc/http)
include_directories(/usr/local/include)

set (LIB_SRC
	 src/log.cpp
	 src/config.cpp
	 src/fiber.cpp
	 src/thread.cpp
	 src/mutex.cpp
	 src/scheduler.cpp
	 src/iomanager.cpp
	 src/fd_manager.cpp
	 src/hook.cpp
	 src/timer.cpp
	 src/util.cpp
	 src/address.cpp
	 src/tcp_server.cpp
	 src/socket.cpp
	 src/bytearray.cpp
	 src/stream.cpp
	 src/uri_rl.cpp
	 src/socket_stream.cpp
	 src/http/http.cpp
	 src/http/http11_parser_rl.cpp
	 src/http/httpclient_parser_rl.cpp
	 src/http/http_parser.cpp
	 src/http/http_session.cpp
	 src/http/http_servlet.cpp
	 src/http/http_server.cpp
	 src/http/http_connection.cpp)


add_library(mnser SHARED ${LIB_SRC})

message("status", "PROJECT_SOURCE_DIR is ${PROJECT_SOURCE_DIR}")

link_directories(${PROJECT_SOURCE_DIR}/lib)
#force_redefine_file_macro_for_sources(mnser)
#find_package(Boost REQUIRED)
#if(Boost_FOUND)
#	include_directories(${Boost_INCLUDE_DIRS})
#endif()

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

set(LIBS 
	mnser 
	pthread 
	yaml-cpp
	dl)

add_executable(test_log "tests/test_log.cpp")
target_link_libraries(test_log ${LIBS})

add_executable(test_thread "tests/test_thread.cpp")
target_link_libraries(test_thread ${LIBS})

add_executable(test_config "tests/test_config.cpp")
target_link_libraries(test_config ${LIBS})

add_executable(test_util "tests/test_util.cpp")
target_link_libraries(test_util ${LIBS})

add_executable(test_scheduler "tests/test_scheduler.cpp")
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_fiber "tests/test_fiber.cpp")
target_link_libraries(test_fiber ${LIBS})

add_executable(test_iomanager "tests/test_iomanager.cpp")
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook "tests/test_hook.cpp")
target_link_libraries(test_hook ${LIBS})

add_executable(test_address "tests/test_address.cpp")
target_link_libraries(test_address ${LIBS})

add_executable(test_socket "tests/test_socket.cpp")
target_link_libraries(test_socket ${LIBS})

add_executable(test_bytearray "tests/test_bytearray.cpp")
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_uri "tests/test_uri.cpp")
target_link_libraries(test_uri ${LIBS})

add_executable(test_http "tests/test_http.cpp")
target_link_libraries(test_http ${LIBS})

add_executable(test_http_parser "tests/test_http_parser.cpp")
target_link_libraries(test_http_parser ${LIBS})

add_executable(test_tcp_server "tests/test_tcp_server.cpp")
target_link_libraries(test_tcp_server ${LIBS})

add_executable(echo_server "examples/echo_server.cpp")
target_link_libraries(echo_server ${LIBS})

add_executable(test_http_server "tests/test_http_server.cpp")
target_link_libraries(test_http_server ${LIBS})

add_executable(test_http_connection "tests/test_http_connection.cpp")
target_link_libraries(test_http_connection ${LIBS})
