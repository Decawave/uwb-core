#project(uwb-core VERSION ${VERSION} LANGUAGES C)

add_subdirectory(json)
add_subdirectory(uwb_ccp)
add_subdirectory(uwb_wcs)
add_subdirectory(tdma)
add_subdirectory(cir)
#add_subdirectory(pan)
add_subdirectory(dsp)
add_subdirectory(euclid)
add_subdirectory(rng_math)
add_subdirectory(uwb_rng)
add_subdirectory(twr_ss)
add_subdirectory(twr_ss_ext)
add_subdirectory(twr_ds)
add_subdirectory(twr_ds_ext)
add_subdirectory(nrng)
add_subdirectory(twr_ss_nrng)
add_subdirectory(survey)
#find_package(timescale CONFIG)

#if(timescale_FOUND)
#add_subdirectory(uwb_wcs)
#endif()



