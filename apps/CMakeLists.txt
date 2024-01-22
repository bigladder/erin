add_executable(test_erin_next
	test_erin_next.cpp
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next.h"
)
target_include_directories(test_erin_next PUBLIC ${PROJECT_SOURCE_DIR}/include)