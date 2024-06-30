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
                     org.cutefish.Session.xml
                     application.h Application
                     sessionadaptor SessionAdaptor)
set_source_files_properties(${DBUS_SOURCES} PROPERTIES SKIP_AUTOGEN ON)

# 查找Qt6的WaylandCompositor组件
find_package(Qt6 COMPONENTS WaylandCompositor REQUIRED)

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

# 安装目标文件到/usr/bin
install(TARGETS ${TARGET} DESTINATION /usr/bin)

# 安装Wayland会话文件
install(FILES prts-wayland.desktop DESTINATION /usr/share/wayland-sessions/)