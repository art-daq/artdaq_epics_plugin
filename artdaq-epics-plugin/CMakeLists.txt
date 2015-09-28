include_directories($ENV{EPICS_INC}/os/Linux)
simple_plugin(epics "metric"
  artdaq-utilities_Plugins
  ${FHICLCPP}
  ${CETLIB}
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
  ${EPICS_CA}
)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
