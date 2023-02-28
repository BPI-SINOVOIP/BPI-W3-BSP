cmake_minimum_required(VERSION 2.8)

set(J2S_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/j2s/cJSON.c
    ${CMAKE_CURRENT_SOURCE_DIR}/j2s/cJSON_Utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/j2s/j2s.c
    ${CMAKE_CURRENT_SOURCE_DIR}/j2s/j2s_utils.c
    )

#########################################################
#               HEADER MACRO PREPROCESS                 #
#########################################################
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/output.h
    COMMAND ${CMAKE_C_COMPILER}
    ${CMAKE_SOURCE_DIR}/include/iq_parser_v2/RkAiqCalibDbTypesV2.h -E -C -o .temp.h ${ISP_HW_VERSION}
    -I${CMAKE_SOURCE_DIR}/
    -I${CMAKE_SOURCE_DIR}/include
    -I${CMAKE_SOURCE_DIR}/include/common
    -I${CMAKE_SOURCE_DIR}/include/uAPI2
    -I${CMAKE_SOURCE_DIR}/include/algos
    -I${CMAKE_SOURCE_DIR}/include/iq_parser_v2
    -I${CMAKE_SOURCE_DIR}/include/iq_parser
    -I${CMAKE_SOURCE_DIR}/include/xcore
    -I${CMAKE_SOURCE_DIR}/include/algos
    COMMAND ${CMAKE_SOURCE_DIR}/tools/iq_check/script/header_strip.sh
    .temp.h output.h
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/RkAiqCalibDbTypesV2_M4.h
    COMMAND cat ${CMAKE_SOURCE_DIR}/tools/iq_check/script/m4/desc.m4
    output.h > RkAiqCalibDbTypesV2.m4
    COMMAND cp ${CMAKE_SOURCE_DIR}/tools/iq_check/script/m4/desc.sh ./
    COMMAND m4 -E RkAiqCalibDbTypesV2.m4 > RkAiqCalibDbTypesV2_M4.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/output.h
)

#########################################################
#                BUILD HOST PARSER tools                 #
#########################################################
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/.j2s_generated.h
    COMMAND gcc -std=gnu99 -DJ2S_ENABLE_DESC ${CMAKE_SOURCE_DIR}/tools/iq_check/src/parser.c -o parser ${ISP_HW_VERSION}
    -I${CMAKE_SOURCE_DIR}/include/common
    -I${CMAKE_SOURCE_DIR}/
    -I${CMAKE_SOURCE_DIR}/include/uAPI2
    -I${CMAKE_SOURCE_DIR}/include/algos
    -I${CMAKE_SOURCE_DIR}/include/iq_parser_v2
    -I${CMAKE_SOURCE_DIR}/include/iq_parser_v2/j2s
    COMMAND ${CMAKE_SOURCE_DIR}/tools/iq_check/script/headerprocess.sh
    ${CMAKE_SOURCE_DIR}/include/iq_parser_v2/RkAiqCalibDbTypesV2.h
    ${CMAKE_BINARY_DIR}/.j2s_generated.h
	#COMMAND ${CMAKE_SOURCE_DIR}/tools/iq_check/bin/parser RkAiqCalibDbTypesV2_M4.h CamCalibDbProj_t >>
	COMMAND ${CMAKE_BINARY_DIR}/iq_parser_v2/parser RkAiqCalibDbTypesV2_M4.h CamCalibDbProj_t >>
    ${CMAKE_BINARY_DIR}/.j2s_generated.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/RkAiqCalibDbTypesV2_M4.h
    )

if(ARCH STREQUAL "aarch64")
	set(HOST_GCC_FLAG "-m64")
else()
	set(HOST_GCC_FLAG "-m32")
endif()
#########################################################
#                    BUILD HOST CODE2BIN tool               #
#########################################################
add_custom_target(
    j2s_code2bin
    COMMAND gcc ${HOST_GCC_FLAG} -malign-double -std=gnu99 ${CMAKE_SOURCE_DIR}/tools/iq_check/src/tobin.c -o tobin ${ISP_HW_VERSION} -DRKAIQ_HAVE_LSC_V3
	-I${CMAKE_BINARY_DIR}
    -I${CMAKE_SOURCE_DIR}/include/common
    -I${CMAKE_SOURCE_DIR}/
    -I${CMAKE_SOURCE_DIR}/include/uAPI2
    -I${CMAKE_SOURCE_DIR}/include/algos
    -I${CMAKE_SOURCE_DIR}/include/iq_parser_v2
    -I${CMAKE_SOURCE_DIR}/include/iq_parser_v2/j2s

	COMMAND ./tobin ${CMAKE_BINARY_DIR}/j2s_code2bin.bin
    COMMAND cd ${CMAKE_BINARY_DIR}/ && xxd -i j2s_code2bin.bin j2s_code2bin.h
    DEPENDS ${CMAKE_BINARY_DIR}/.j2s_generated.h
    )

#########################################################
#                    BUILD HOST j2s_generated.h optimize tool               #
#########################################################
add_custom_target(
    j2s_generated_optim
    COMMAND gcc ${HOST_GCC_FLAG} -malign-double -std=gnu99 ${CMAKE_SOURCE_DIR}/tools/iq_check/src/j2s_generated_optim.c -o j2s_generated_optim ${ISP_HW_VERSION}
	-I${CMAKE_BINARY_DIR}
    -I${CMAKE_SOURCE_DIR}/include/common
    -I${CMAKE_SOURCE_DIR}/
    -I${CMAKE_SOURCE_DIR}/include/uAPI2
    -I${CMAKE_SOURCE_DIR}/include/algos
    -I${CMAKE_SOURCE_DIR}/include/iq_parser_v2
    -I${CMAKE_SOURCE_DIR}/include/iq_parser_v2/j2s

	COMMAND ./j2s_generated_optim ${CMAKE_BINARY_DIR}/j2s_generated.h
    DEPENDS j2s_code2bin
    )

#########################################################
#                    BUILD HOST DUMP tool               #
#########################################################
add_custom_target(
    j2s_parser
    COMMAND gcc -std=gnu99 -DJ2S_ENABLE_DESC -DCOMPILE_TEMPLATE -o dump ${J2S_SRC} ${ISP_HW_VERSION}
    ${CMAKE_SOURCE_DIR}/tools/iq_check/src/dump.c
    -I${CMAKE_SOURCE_DIR}/include/common
    -I${CMAKE_SOURCE_DIR}/
    -I${CMAKE_SOURCE_DIR}/include/uAPI2
    -I${CMAKE_SOURCE_DIR}/include/algos
    -I${CMAKE_SOURCE_DIR}/include/iq_parser_v2
    -I${CMAKE_SOURCE_DIR}/include/iq_parser_v2/j2s
    -I${PROJECT_BINARY_DIR}

    COMMAND ./dump --template > .j2s_template.json
    COMMAND cp ${CMAKE_SOURCE_DIR}/tools/iq_check/script/dump_template.sh ./
    COMMAND ./dump_template.sh
    ${CMAKE_BINARY_DIR}/j2s_generated_template_json.h
	DEPENDS ${CMAKE_BINARY_DIR}/.j2s_generated.h
	DEPENDS j2s_generated_optim
    )

#########################################################
#                    BUILD BINARY IQ FILES              #
#########################################################
add_custom_target(
    j2s4b
    COMMAND ${CMAKE_COMMAND}
    -DROOT_PROJ_BIN_DIR:STRING=${CMAKE_BINARY_DIR}
    -DRKAIQ_IQFILES_DIR:STRING="${CMAKE_SOURCE_DIR}/iqfiles/isp3x"
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE:PATH="${CMAKE_BINARY_DIR}/bin"
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
    -B"host_tool"
    -H"${CMAKE_SOURCE_DIR}/tools/j2s4b"
    COMMAND ${CMAKE_COMMAND} --build host_tool
    COMMAND ${CMAKE_COMMAND} --install host_tool
	DEPENDS j2s_parser
    )

SET(IQ_PARSERV2_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/j2s/cJSON.c
    ${CMAKE_CURRENT_SOURCE_DIR}/j2s/cJSON_Utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/j2s/j2s.c
    ${CMAKE_CURRENT_SOURCE_DIR}/j2s/j2s_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/RkAiqCalibDbV2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene_manager.cpp
    )

SET(RK_AIQ_SRC ${RK_AIQ_SRC} ${IQ_PARSERV2_SRC} PARENT_SCOPE)
