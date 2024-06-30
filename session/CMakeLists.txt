project(prts-session)
set(TARGET prts-session)

set(SOURCES
    application.cpp
    main.cpp
    process.cpp
    processmanager.cpp
    powermanager/power.cpp
    powermanager/powerproviders.cpp
)

qt_add_dbus_adaptor(DBUS_SOURCES
    org.prts.Session.xml
    application.h Application
    sessionadaptor SessionAdaptor
)
set_source_files_properties(${DBUS_SOURCES} PROPERTIES SKIP_AUTOGEN ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick DBus WaylandCompositor)
find_package(KF6WindowSystem REQUIRED)

add_executable(${TARGET} ${SOURCES} ${DBUS_SOURCES})
target_link_libraries(${TARGET}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::DBus
    Qt6::WaylandCompositor
    KF6::WindowSystem
)

install(TARGETS ${TARGET} DESTINATION /usr/bin)
install(FILES prts-wayland.desktop DESTINATION /usr/share/wayland-sessions/)
