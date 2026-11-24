# pxrender.dll: PrimeXT's renderer (client/render) as a module that reTS's client.dll loads when r_primext is 1.
# H9: written new. The sources under this folder are PrimeXT's (see reTS/PRIMEXT-SOURCES.md), built with PrimeXT's
# own headers (common/, engine/, game_shared/, public/ here), never with reTS's: the two HLSDK forks differ in
# Vector, mnode_t and the engine headers, and the module keeps them apart. Only primext_bridge.h is shared.
# Windows (MSVC, 32-bit) only for now: the web build and Linux keep today's drawing.

set(PXR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB PXR_RENDER_SOURCES ${PXR_DIR}/client/render/*.cpp)
file(GLOB PXR_ENTITY_SOURCES ${PXR_DIR}/client/entities/*.cpp)

set(PXR_SOURCES
	${PXR_RENDER_SOURCES}
	${PXR_ENTITY_SOURCES}
	${PXR_DIR}/bridge/pxr_main.cpp
	${PXR_DIR}/bridge/pxr_shells.cpp
	${PXR_DIR}/client/utils.cpp
	${PXR_DIR}/client/parsemsg.cpp
	${PXR_DIR}/client/postfx_controller.cpp
	${PXR_DIR}/client/postfx_parameters.cpp
	${PXR_DIR}/client/visualizer/debug_visualizer.cpp
	${PXR_DIR}/game_shared/bone_setup.cpp
	${PXR_DIR}/game_shared/common.cpp
	${PXR_DIR}/game_shared/ikcontext.cpp
	${PXR_DIR}/game_shared/jigglebones.cpp
	${PXR_DIR}/game_shared/material.cpp
	${PXR_DIR}/game_shared/mathlib.cpp
	${PXR_DIR}/game_shared/matrix.cpp
	${PXR_DIR}/game_shared/bounding_box.cpp
	${PXR_DIR}/game_shared/frustum.cpp
	${PXR_DIR}/game_shared/meshdesc.cpp
	${PXR_DIR}/game_shared/meshdesc_factory.cpp
	${PXR_DIR}/game_shared/procbones.cpp
	${PXR_DIR}/game_shared/stringlib.cpp
	${PXR_DIR}/game_shared/virtualfs.cpp
	${PXR_DIR}/game_shared/trace.cpp
	${PXR_DIR}/game_shared/filesystem_utils.cpp
	${PXR_DIR}/game_shared/filesystem_manager.cpp
	${PXR_DIR}/public/crclib.cpp
)

add_library(pxrender SHARED ${PXR_SOURCES})

target_include_directories(pxrender PRIVATE
	${PXR_DIR}/bridge
	${PXR_DIR}/client
	${PXR_DIR}/client/render
	${PXR_DIR}/client/entities
	${PXR_DIR}/common
	${PXR_DIR}/engine
	${PXR_DIR}/game_shared
	${PXR_DIR}/public)

target_compile_definitions(pxrender PRIVATE
	CLIENT_WEAPONS=1
	CLIENT_DLL=1
	_CRT_SECURE_NO_WARNINGS=1
	_CRT_NONSTDC_NO_DEPRECATE=1
	_CRT_SILENCE_NONCONFORMING_TGMATH_H)

set_target_properties(pxrender PROPERTIES
	PREFIX ""
	OUTPUT_NAME "pxrender"
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
	target_compile_options(pxrender PRIVATE /MP /W3 $<$<CONFIG:Release,RelWithDebInfo>:/O2> /Zi)
	target_link_options(pxrender PRIVATE /DEBUG /OPT:REF /OPT:ICF)
	target_link_libraries(pxrender PRIVATE user32.lib)
endif()
