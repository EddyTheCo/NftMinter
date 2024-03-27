cmake_minimum_required(VERSION 3.24)

set(USE_QML ON)
include(${CMAKE_CURRENT_BINARY_DIR}/local_conf.cmake OPTIONAL)

include(FetchContent)
FetchContent_Declare(
	ccommon
	GIT_REPOSITORY https://github.com/EddyTheCo/Common.git
	GIT_TAG use_IFW
	)
FetchContent_MakeAvailable(ccommon)
version_from_git(
	LOG       OFF
	TIMESTAMP "%Y%m%d%H%M%S"
	)
project(IotaNFTMinter VERSION ${VERSION} DESCRIPTION "NFT Minter for Iota network" LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
include(GNUInstallDirs)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml OPTIONAL_COMPONENTS Multimedia)

FetchContent_Declare(
	EstervDesigns
	GIT_REPOSITORY https://github.com/EddyTheCo/MyDesigns.git
	GIT_TAG use_IFW
	FIND_PACKAGE_ARGS 1.1 COMPONENTS SimpleStyle CustomControls CONFIG
	)
FetchContent_MakeAvailable(EstervDesigns)

FetchContent_Declare(
	IotaWallet
	GIT_REPOSITORY https://github.com/EddyTheCo/qWallet-IOTA.git
	GIT_TAG use_IFW
	FIND_PACKAGE_ARGS 0.2 CONFIG
	)
FetchContent_MakeAvailable(IotaWallet)


FetchContent_Declare(
	qrCode
	GIT_REPOSITORY https://github.com/EddyTheCo/qrCode.git
	GIT_TAG use_IFW
	FIND_PACKAGE_ARGS 1.0 COMPONENTS QtQrDec QtQrGen CONFIG
	)
FetchContent_MakeAvailable(qrCode)

FetchContent_Declare(
	DTPickersQML
	GIT_REPOSITORY https://github.com/EddyTheCo/DateTimePickers.git
	GIT_TAG use_IFW
	FIND_PACKAGE_ARGS 0.1 CONFIG
	)
FetchContent_MakeAvailable(DTPickersQML)


if(EMSCRIPTEN OR ANDROID)
	find_package(Qt6 REQUIRED COMPONENTS QuickControls2)
	find_package(EstervDesigns 0.4 REQUIRED COMPONENTS FlatControl CONFIG )
endif(EMSCRIPTEN OR ANDROID)

qt_standard_project_setup()

qt_add_executable(nftminter main.cpp)

set(updateFrame "qml/empty/UpdateFrame.qml")
if(QTDEPLOY)
    set(updateFrame "qml/UpdateFrame.qml")
endif(QTDEPLOY)

qt6_add_qml_module(nftminter
	URI  Esterv.Iota.NFTMinter
	VERSION 1.0
	SOURCES
	src/CreateNFT.cpp include/CreateNFT.hpp
	QML_FILES
	"qml/SendDialog.qml"
	"qml/BoxNFT.qml"
	"qml/BoxMenu.qml"
	"qml/ConfDrawer.qml"
	"qml/window.qml"
        ${updateFrame}
	RESOURCE_PREFIX
	"/esterVtech.com/imports"
	RESOURCES
	"fonts/Anton/Anton-Regular.ttf"
	"fonts/Permanent_Marker/PermanentMarker-Regular.ttf"
	IMPORT_PATH ${CMAKE_BINARY_DIR}
	)


target_link_libraries(nftminter PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick )

if(EMSCRIPTEN OR ANDROID)
	target_link_libraries(nftminter PRIVATE EstervDesigns::FlatControl Qt6::QuickControls2
		$<$<STREQUAL:$<TARGET_PROPERTY:EstervDesigns::FlatControl,TYPE>,STATIC_LIBRARY>:EstervDesigns::FlatControlplugin>
		)
	target_compile_definitions(nftminter PRIVATE FORCE_STYLE="Esterv.Controls.Flat")
endif(EMSCRIPTEN OR ANDROID)

if(TARGET Qt6::Multimedia)
	target_link_libraries(nftminter PRIVATE Qt6::Multimedia)
endif(TARGET Qt6::Multimedia)

if(ANDROID)
	set_property(TARGET nftminter PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
	set_property(TARGET nftminter APPEND PROPERTY QT_ANDROID_MIN_SDK_VERSION 30)
	set_property(TARGET nftminter APPEND PROPERTY QT_ANDROID_TARGET_SDK_VERSION 34)
	set_property(TARGET nftminter APPEND PROPERTY QT_ANDROID_SDK_BUILD_TOOLS_REVISION 34.0.0)

	FetchContent_Declare(
		android_openssl
		DOWNLOAD_EXTRACT_TIMESTAMP true
		URL      https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
		)
	FetchContent_GetProperties(android_openssl)
	if(NOT android_openssl_POPULATED)
		FetchContent_Populate(android_openssl)
		include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
		add_android_openssl_libraries(nftminter)
	endif(NOT android_openssl_POPULATED)

endif(ANDROID)

target_link_libraries(nftminter PRIVATE IotaWallet::qwallet qrCode::QtQrGen qrCode::QtQrDec  DTPickersQML::DTPickers
	$<$<STREQUAL:$<TARGET_PROPERTY:DTPickersQML::DTPickers,TYPE>,STATIC_LIBRARY>:DTPickersQML::DTPickersplugin>
	$<$<STREQUAL:$<TARGET_PROPERTY:qrCode::QtQrGen,TYPE>,STATIC_LIBRARY>:qrCode::QtQrGenplugin>
        $<$<STREQUAL:$<TARGET_PROPERTY:IotaWallet::qwallet,TYPE>,STATIC_LIBRARY>:IotaWallet::qwalletplugin>
	$<$<STREQUAL:$<TARGET_PROPERTY:qrCode::QtQrDec,TYPE>,STATIC_LIBRARY>:qrCode::QtQrDecplugin>
	)
set_target_properties(nftminter PROPERTIES
	WIN32_EXECUTABLE ON
	MACOSX_BUNDLE ON
	)


target_include_directories(nftminter PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

install(TARGETS nftminter
        COMPONENT Nftminter
        BUNDLE  DESTINATION . COMPONENT Nftminter
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
if(QTDEPLOY)
    FetchContent_Declare(
            QtUpdater
            GIT_REPOSITORY https://github.com/EddyTheCo/QUpdater.git
            GIT_TAG use_IFW
            FIND_PACKAGE_ARGS 0.1 CONFIG
            )
    FetchContent_MakeAvailable(QtUpdater)

    target_link_libraries(nftminter PRIVATE QtUpdater::qupdater
        $<$<STREQUAL:$<TARGET_PROPERTY:QtUpdater::qupdater,TYPE>,STATIC_LIBRARY>:QtUpdater::updaterplugin>
    )
	qt_generate_deploy_qml_app_script(
		TARGET nftminter
		OUTPUT_SCRIPT deploy_script
		)
        install(SCRIPT ${deploy_script} COMPONENT Nftminter)
	target_compile_definitions(nftminter PRIVATE USE_UPDATER)

endif(QTDEPLOY)
if(EMSCRIPTEN)
	target_compile_definitions(nftminter PRIVATE USE_EMSCRIPTEN)
	add_custom_command(
		TARGET nftminter
		POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy "${CMAKE_CURRENT_BINARY_DIR}/nftminter.js" "${CMAKE_CURRENT_BINARY_DIR}/nftminter.wasm" "${CMAKE_CURRENT_BINARY_DIR}/qtloader.js" "${CMAKE_CURRENT_SOURCE_DIR}/wasm"
		)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
		build_cpack()
endif(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
