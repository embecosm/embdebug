set(INSTALL_HEADERS Init.h
                    ITarget.h
                    MemMap.h
                    Timeout.h
                    TraceFlags.h
                    Types.h
                    Utils.h)

install(FILES ${INSTALL_HEADERS} DESTINATION include/embdebug)
