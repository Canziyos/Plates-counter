idf_component_register(SRCS "main.cpp" "tasks.cpp" "dish_counter_logic.cpp" "lorawan_service.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES ttn-esp32)
