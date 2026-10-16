include("${PROJECT_SOURCE_DIR}/cmake/tests.cmake")

ouasim_configure_shared_test_objects()

ouasim_add_test(ouasim_tests_formatting         test_formatting.cpp)
ouasim_add_test(ouasim_tests_tree_model         test_treemodel.cpp)
ouasim_add_test(ouasim_tests_table_models       test_tablemodels.cpp)
ouasim_add_test(ouasim_tests_log_filter         test_logfilterproxymodel.cpp)
ouasim_add_test(ouasim_tests_tree_builders      test_treebuilders.cpp)
ouasim_add_test(ouasim_tests_mock_data          test_mockdata.cpp)
ouasim_add_test(ouasim_tests_quaserver_smoke   test_quaserver_smoke.cpp)
ouasim_add_test(ouasim_tests_signal_generator  test_signalgenerator.cpp)
ouasim_add_test(ouasim_tests_value_cast        test_valuecast.cpp)
ouasim_add_test(ouasim_tests_project_file      test_projectfile.cpp)

ouasim_add_ui_test(ouasim_tests_app_theme       test_apptheme.cpp)
ouasim_add_ui_test(ouasim_tests_widgets         test_widgets.cpp)
ouasim_add_ui_test(ouasim_tests_pages           test_pages.cpp)
ouasim_add_ui_test(ouasim_tests_mainwindow      test_mainwindow.cpp)
