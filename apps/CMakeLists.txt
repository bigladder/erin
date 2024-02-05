add_executable(test_erin_next
	test_erin_next.cpp
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_distribution.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_reliability.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_timestate.h"
)
target_include_directories(test_erin_next PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_erin_next erin_next)

add_executable(erin_next_stress_test
	erin_next_stress_test.cpp
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_distribution.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_reliability.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_timestate.h"
)
target_include_directories(erin_next_stress_test
	PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(erin_next_stress_test erin_next)