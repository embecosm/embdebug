set(INSTALL_HEADERS ITarget.h
                    MemMap.h
                    RegisterSizes.h
                    Timeout.h
                    TraceFlags.h
                    Utils.h)

install(FILES ${INSTALL_HEADERS} DESTINATION include/embdebug)
