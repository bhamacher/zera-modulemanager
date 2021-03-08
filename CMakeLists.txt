cmake_minimum_required(VERSION 3.1)

#set project name and language
project(zera-modulemanager LANGUAGES CXX)

#Automoc and RCC ON. Needed in QT projects
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#C++ 11 Flag
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Set project Version
set(zeramodulemanager_VERSION_MAJOR "1")
set(zeramodulemanager_VERSION_MINOR "2")
set(zeramodulemanager_VERSION_PATCH "0")

set(PROJECT_VERSION "${zeramodulemanager_VERSION_MAJOR}.${zeramodulemanager_VERSION_MINOR}.${zeramodulemanager_VERSION_PATCH}")

#Adding cmake functions like feature_summary()
include(FeatureSummary)
include(GNUInstallDirs)

#Find dependecies
find_package(Qt5 COMPONENTS Core Network Qml Concurrent CONFIG REQUIRED)
find_package(VfEvent REQUIRED)
find_package(VfComponent REQUIRED)
find_package(VfStorageHash REQUIRED)
find_package(VfTcp REQUIRED)
find_package(VfNet2 REQUIRED)
find_package(VfHelpers REQUIRED)
#find_package(VfQml REQUIRED)
find_package(VfScript REQUIRED)
find_package(VfLogger REQUIRED)
find_package(VfCryptoBridge REQUIRED)
find_package(VfExport REQUIRED)
find_package(VfFiles REQUIRED)
find_Package(MeasurementModuleInterface REQUIRED)
find_Package(zeraproxy REQUIRED)
find_Package(VfCpp REQUIRED)
find_package(VfVeinmanager REQUIRED)
find_package(VfSession REQUIRED)
find_package(VfCustomerdata REQUIRED)

option(MM_NO_CONFIG_INSTALL "Do not install configuration/session files - for debugging on PC" OFF)

set(SET_MODMAN_SESSION_PATH "${CMAKE_INSTALL_SYSCONFDIR}/zera/modules/sessions/")
set(SET_MODMAN_CONFIG_PATH  "${CMAKE_INSTALL_SYSCONFDIR}/zera/modules/")
set(SET_MODMAN_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/zera-modules/")
set(SET_MODMAN_CONTENTSET_PATH "${CMAKE_INSTALL_SYSCONFDIR}/zera/configs/")
set(SET_MODMAN_CUSTOMERDATA_PATH "/home/operator/customerdata/" CACHE STRING "Customer data path")
set(SET_MODMAN_CUST_CONTENTSET_PATH "/home/operator/logger-contentsets/" CACHE STRING "Logger contentset path")
set(SET_MODMAN_AUTOMOUNT_PATH "/media/" CACHE STRING "Automount path")
set(SET_MODMAN_LOGGER_LOCAL_PATH "/home/operator/logger" CACHE STRING "Logger local storage path")

add_definitions(-DMODMAN_SESSION_PATH="${SET_MODMAN_SESSION_PATH}")
add_definitions(-DMODMAN_CONFIG_FILE="${SET_MODMAN_CONFIG_PATH}/modulemanager_config.json")
add_definitions(-DMODMAN_MODULE_PATH="${SET_MODMAN_MODULE_PATH}")
add_definitions(-DMODMAN_CUSTOMERDATA_PATH="${SET_MODMAN_CUSTOMERDATA_PATH}")
#add_definitions(-DSYSCONFDIR="${SET_MODMAN_CONFIG_PATH}")
add_definitions(-DMODMAN_CONTENTSET_PATH="${SET_MODMAN_CONTENTSET_PATH}")
add_definitions(-DMODMAN_CUST_CONTENTSET_PATH="${SET_MODMAN_CUST_CONTENTSET_PATH}")
add_definitions(-DMODMAN_AUTOMOUNT_PATH="${SET_MODMAN_AUTOMOUNT_PATH}")
add_definitions(-DMODMAN_LOGGER_LOCAL_PATH="${SET_MODMAN_LOGGER_LOCAL_PATH}")

get_directory_property( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
foreach( d ${DirDefs} )
    message( STATUS "Found Define: " ${d} )
endforeach()


#sum up project Files 
file(GLOB HEADER
${PROJECT_SOURCE_DIR}/src/*.h
    )


file(GLOB SOURCES
${PROJECT_SOURCE_DIR}/src/*.cpp
)

file(GLOB RESOURCES 
    ${PROJECT_SOURCE_DIR}/*.qrc
    )

set(SESSION_FILES
    ${PROJECT_SOURCE_DIR}/target/com5003-meas-session.json 
    ${PROJECT_SOURCE_DIR}/target/com5003-ref-session.json 
    ${PROJECT_SOURCE_DIR}/target/com5003-ced-session.json 
    ${PROJECT_SOURCE_DIR}/target/mt310s2-meas-session.json
)

set(CONFIG_FILES
    ${PROJECT_SOURCE_DIR}/modulemanager_config.json
)


#create executable
add_executable(zera-modulemanager
    ${SOURCES}
    ${HEADER}
    ${RESOURCES}
    )


#link dependencies
target_link_libraries(zera-modulemanager
    PRIVATE
    Qt5::Core
    Qt5::Network
    Qt5::Concurrent
    VeinMeta::VfEvent
    VeinMeta::VfComponent
    VeinMeta::VfStorageHash
    VeinMeta::VfTcp
    VeinMeta::VfNet2
    VeinMeta::VfHelpers
    VeinMeta::VfQml
    VeinMeta::VfScript
    VeinMeta::VfLogger
    VeinMeta::VfCpp
    VeinMeta::VfExport
    VeinMeta::VfFiles
    VeinMeta::VfCryptoBridge
    ZeraClasses::MeasurementModuleInterface
    ZeraClasses::zeraproxy
    VeinMeta::VfVeinmanager
    VeinMeta::VfSession
    VeinMeta::VfCustomerdata
)	





# announce headers - target perspective
target_include_directories(zera-modulemanager
    PUBLIC
    $<BUILD_INTERFACE:${MY_LIBRARY_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/zera-modulemanager>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    )



#install zera-modulemanagerto bin 
install(TARGETS zera-modulemanager
    EXPORT zera-modulemanagerExport
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

if(NOT MM_NO_CONFIG_INSTALL)
    install(
        FILES ${SESSION_FILES}
        DESTINATION ${SET_MODMAN_SESSION_PATH}
        )

    install(
        FILES ${CONFIG_FILES}
        DESTINATION ${SET_MODMAN_CONFIG_PATH}
        )
endif()

# spawn out some info on configuration
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)






