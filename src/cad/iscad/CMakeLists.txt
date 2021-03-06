if (INSIGHT_BUILD_ISCAD)

project(iscad)

set(iscad_SRCS 
 iscad.cpp
 iscadmainwindow.cpp
 iscadsyntaxhighlighter.cpp
 iscadapplication.cpp
 iscadmodel.cpp

 modelcomponentselectordlg.cpp
 insertfeaturedlg.cpp

 bgparsingthread.cpp
)
find_package(Qt4 REQUIRED)
find_package(Qwt REQUIRED)
#find_package(Boost COMPONENTS system filesystem REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
#include_directories(${toolkit_SOURCE_DIR})
# link_directories(${toolkit_BIN_DIR})

SET(iscad_FORMS modelcomponentselectordlg.ui insertfeaturedlg.ui) #newanalysisdlg.ui analysisform.ui)

SET(iscad_RCCS iscad.qrc)

QT4_WRAP_UI(iscad_FORMS_HEADERS ${iscad_FORMS})
qt4_automoc(${iscad_SRCS})

add_executable(iscad ${iscad_SRCS} ${iscad_FORMS_HEADERS} ${iscad_RCCS})
target_link_libraries(iscad insightcad insightcad_gui ${OCC_LIBRARIES} ${OCC_OCAF_LIBRARIES} ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES} toolkit toolkit_gui
  Qt4::QtCore Qt4::QtGui Qt4::QtWebKit
  ${QWT_LIBRARY} ${QWT_LIBRARIES}
  ${DXFLIB_LIBRARIES}
  )
install(TARGETS iscad RUNTIME DESTINATION bin)  
endif()
