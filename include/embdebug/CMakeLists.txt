set(INSTALL_HEADERS ByteView.h
                    Compat.h
                    Init.h
                    ITarget.h
                    Timeout.h
                    TraceFlags.h
                    Types.h
                    Utils.h)

install(FILES ${INSTALL_HEADERS} DESTINATION include/embdebug)
