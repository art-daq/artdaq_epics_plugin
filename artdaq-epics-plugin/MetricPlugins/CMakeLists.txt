include_directories($ENV{EPICS_INC}/compiler/gcc)
include_directories($ENV{EPICS_INC}/os/Linux)

basic_plugin(epics "metric"
  artdaq_utilities::artdaq-utilities_Plugins
  fhiclcpp
  cetlib
  cetlib_except
  MF_MessageLogger
  ca
)

install_fhicl(LIST fcl/epics.fcl)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

install_headers()
install_source()
