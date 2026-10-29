function(ouasim_dependency_revision source_dir version output_variable)
    string(STRIP "${version}" revision)
    if(revision STREQUAL "")
        find_package(Git QUIET)
        if(GIT_FOUND)
            file(REAL_PATH "${source_dir}" source_root)
            execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${source_root}" rev-parse --show-toplevel
                OUTPUT_VARIABLE git_root OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            if(NOT git_root STREQUAL "")
                file(REAL_PATH "${git_root}" git_root)
                # Archives inside the application checkout must not inherit its commit.
                if(git_root STREQUAL source_root)
                    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${source_root}" rev-parse --short=12 HEAD
                        RESULT_VARIABLE result OUTPUT_VARIABLE commit
                        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
                    if(result EQUAL 0)
                        set(revision "${commit}")
                    endif()
                endif()
            endif()
        endif()
    endif()
    set(${output_variable} "${revision}" PARENT_SCOPE)
endfunction()
