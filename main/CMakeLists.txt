idf_component_register(SRCS "main.cpp" 
                            "lib/tool/tool.cpp" 
                            "lib/senaryo/senaryo.cpp"
                            "ping.cpp"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ca_cert.pem)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)                    