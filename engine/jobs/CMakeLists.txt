# Jobs library

target_sources(greywater_core
    PRIVATE
        scheduler.cpp
        worker.cpp
        job.cpp
        job_queue.cpp
        wait_group.cpp
        parallel_for.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES
            scheduler.hpp
            worker.hpp
            job.hpp
            job_queue.hpp
            wait_group.hpp
            parallel_for.hpp
            reserved_worker.hpp
)
