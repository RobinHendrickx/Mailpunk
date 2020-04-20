project(MailPunk)
cmake_minimum_required(VERSION 3.10)
include(ExternalProject)
link_directories(MailPunk ${MailPunk_BINARY_DIR}/deps/lib)

add_executable(MailPunk MailPunk.cpp imap.cpp UI.cpp)
set_property(TARGET MailPunk PROPERTY CXX_STANDARD 17)

ExternalProject_Add(libfinal
  URL "https://github.com/gansm/finalcut/archive/0.5.0.tar.gz"
  PATCH_COMMAND     "./autogen.sh"
  CONFIGURE_COMMAND "./configure" --prefix ${MailPunk_BINARY_DIR}/deps
  BUILD_COMMAND     "make" COMMAND make install
  BUILD_IN_SOURCE   TRUE
  BUILD_ALWAYS      FALSE
  )

ExternalProject_Add(libetpan
	URL "https://github.com/dinhviethoa/libetpan/archive/1.9.1.tar.gz"
	PATCH_COMMAND     "./autogen.sh"
  CONFIGURE_COMMAND "./configure" --prefix ${MailPunk_BINARY_DIR}/deps
  BUILD_COMMAND     "make" COMMAND make install
	BUILD_IN_SOURCE   TRUE
	BUILD_ALWAYS      FALSE
  )

target_include_directories(MailPunk SYSTEM PUBLIC ${MailPunk_BINARY_DIR}/deps/include)

add_dependencies(MailPunk libetpan)
target_link_libraries(MailPunk etpan)
add_dependencies(MailPunk libfinal)
target_link_libraries(MailPunk final)


