SET(${PROJECT_NAME}_PYTHON_FILES
  __init__.py
  )

FOREACH(PYTHON_FILE ${${PROJECT_NAME}_PYTHON_FILES})
  PYTHON_BUILD(. ${PYTHON_FILE})
  INSTALL(FILES ${PYTHON_FILE} DESTINATION ${${PY_NAME}_INSTALL_DIR})
ENDFOREACH(PYTHON_FILE ${${PROJECT_NAME}_PYTHON_FILES})
