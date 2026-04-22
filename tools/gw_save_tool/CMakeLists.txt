add_executable(gw_save_tool main.cpp)
target_compile_features(gw_save_tool PRIVATE cxx_std_23)
target_link_libraries(gw_save_tool PRIVATE greywater::persist)
