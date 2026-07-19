set(VALLEY_DIR "${CMAKE_INSTALL_DIR}")

find_package(flatbuffers REQUIRED)

include(${FlatBuffers_CMAKE_DIR}/BuildFlatBuffers.cmake OPTIONAL)

set(message_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common/message)
set(foxglove_flat_SRCS
    ${message_DIR}/foxglove/flatbuffer/ArrowPrimitive.fbs
    ${message_DIR}/foxglove/flatbuffer/ArrowPrimitive.fbs
    ${message_DIR}/foxglove/flatbuffer/ByteVector.fbs
    ${message_DIR}/foxglove/flatbuffer/CameraCalibration.fbs
    ${message_DIR}/foxglove/flatbuffer/CircleAnnotation.fbs
    ${message_DIR}/foxglove/flatbuffer/Color.fbs
    ${message_DIR}/foxglove/flatbuffer/CompressedAudio.fbs
    ${message_DIR}/foxglove/flatbuffer/CompressedImage.fbs
    ${message_DIR}/foxglove/flatbuffer/CompressedPointCloud.fbs
    ${message_DIR}/foxglove/flatbuffer/CompressedVideo.fbs
    ${message_DIR}/foxglove/flatbuffer/CubePrimitive.fbs
    ${message_DIR}/foxglove/flatbuffer/CylinderPrimitive.fbs
    ${message_DIR}/foxglove/flatbuffer/Duration.fbs
    ${message_DIR}/foxglove/flatbuffer/FrameTransform.fbs
    ${message_DIR}/foxglove/flatbuffer/FrameTransforms.fbs
    ${message_DIR}/foxglove/flatbuffer/GeoJSON.fbs
    ${message_DIR}/foxglove/flatbuffer/Grid.fbs
    ${message_DIR}/foxglove/flatbuffer/ImageAnnotations.fbs
    ${message_DIR}/foxglove/flatbuffer/Imu.fbs
    ${message_DIR}/foxglove/flatbuffer/JointState.fbs
    ${message_DIR}/foxglove/flatbuffer/JointStates.fbs
    ${message_DIR}/foxglove/flatbuffer/KeyValuePair.fbs
    ${message_DIR}/foxglove/flatbuffer/LaserScan.fbs
    ${message_DIR}/foxglove/flatbuffer/LinePrimitive.fbs
    ${message_DIR}/foxglove/flatbuffer/LocationFix.fbs
    ${message_DIR}/foxglove/flatbuffer/LocationFixes.fbs
    ${message_DIR}/foxglove/flatbuffer/Log.fbs
    ${message_DIR}/foxglove/flatbuffer/ModelPrimitive.fbs
    ${message_DIR}/foxglove/flatbuffer/Odometry.fbs
    ${message_DIR}/foxglove/flatbuffer/PackedElementField.fbs
    ${message_DIR}/foxglove/flatbuffer/Point2.fbs
    ${message_DIR}/foxglove/flatbuffer/Point3.fbs
    ${message_DIR}/foxglove/flatbuffer/Point3InFrame.fbs
    ${message_DIR}/foxglove/flatbuffer/PointCloud.fbs
    ${message_DIR}/foxglove/flatbuffer/PointsAnnotation.fbs
    ${message_DIR}/foxglove/flatbuffer/Pose.fbs
    ${message_DIR}/foxglove/flatbuffer/PoseInFrame.fbs
    ${message_DIR}/foxglove/flatbuffer/PosesInFrame.fbs
    ${message_DIR}/foxglove/flatbuffer/Quaternion.fbs
    ${message_DIR}/foxglove/flatbuffer/RawAudio.fbs
    ${message_DIR}/foxglove/flatbuffer/RawImage.fbs
    ${message_DIR}/foxglove/flatbuffer/SceneEntity.fbs
    ${message_DIR}/foxglove/flatbuffer/SceneEntityDeletion.fbs
    ${message_DIR}/foxglove/flatbuffer/SceneUpdate.fbs
    ${message_DIR}/foxglove/flatbuffer/SpherePrimitive.fbs
    ${message_DIR}/foxglove/flatbuffer/TextAnnotation.fbs
    ${message_DIR}/foxglove/flatbuffer/TextPrimitive.fbs
    ${message_DIR}/foxglove/flatbuffer/Time.fbs
    ${message_DIR}/foxglove/flatbuffer/TriangleListPrimitive.fbs
    ${message_DIR}/foxglove/flatbuffer/Vector2.fbs
    ${message_DIR}/foxglove/flatbuffer/Vector3.fbs
    ${message_DIR}/foxglove/flatbuffer/VoxelGrid.fbs
)

flatbuffers_generate_headers(
    TARGET foxglove_flat
    SCHEMAS ${foxglove_flat_SRCS}
    #INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/schemas
    #FLATC_ARGS --gen-mutable --gen-object-api
)

set(sensor_SRCS 
    sensor/camera_device.h
    sensor/camera_device.cpp
    sensor/imu_device.h
    sensor/imu_device.cpp
    sensor/lidar_device.h
    sensor/lidar_device.cpp)

ADD_EXECUTABLE(valley_drive "main.cpp" ${common_SRCS} ${sensor_SRCS})
target_include_directories(valley_drive 
    PRIVATE 
        . 
        ${VALLEY_MODULES_DIR}
        )

target_link_libraries(valley_drive 
    PRIVATE 
    vIPC
    foxglove_flat
    flatbuffers::flatbuffers)

add_dependencies(valley_drive foxglove_flat)