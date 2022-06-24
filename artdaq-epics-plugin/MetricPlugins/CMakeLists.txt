
include(MetricPlugins)

cet_build_plugin(epics artdaq::metric LIBRARIES REG EPICS::ca TRACE::MF)

install_fhicl(LIST fcl/epics.fcl)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
