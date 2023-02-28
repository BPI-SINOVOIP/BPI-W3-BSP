set(AF_SRC
    rk_aiq_algo_af_itf.cpp
    af.cpp
    af_search.cpp
    af_trigger.cpp
    af_utils.cpp
    rk_aiq_uapi_af_int.cpp
    )

dec_algo(af rk_aiq_algo_af_itf.cpp ${AF_SRC})

if(PDAF_SUPPORT)
    file(COPY ${PREFIX}rkaiq_pdaf.${SUFFIX}	DESTINATION ${LIBRARY_OUTPUT_PATH})
    add_library(rkaiq_pdaf STATIC IMPORTED GLOBAL)
    set_target_properties(rkaiq_pdaf PROPERTIES IMPORTED_LOCATION 	"${PREFIX}rkaiq_pdaf.${SUFFIX}")
    set_target_properties(rkaiq_pdaf PROPERTIES IMPORTED_IMPLIB 	"${PREFIX}rkaiq_pdaf.${SUFFIX}")
endif()
