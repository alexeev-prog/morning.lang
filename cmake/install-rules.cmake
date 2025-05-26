install(
    TARGETS morninglang_exe
    RUNTIME COMPONENT morninglang_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
