#ifndef ROUTE_VIEWER_H
#define ROUTE_VIEWER_H

#include "settings.h"

#include <vsg/core/Array.h>
#include <vsg/core/ref_ptr.h>
#include <vsg/maths/quat.h>
#include <vsg/maths/sphere.h>
#include <vsg/maths/vec3.h>
#include <vsg/nodes/Group.h>
#include <memory>
#include <mutex>
#include <vector>

class  CfgReader;
class  FileSystem;
class  NewSkybox;
struct GUIParams;
class  QByteArray;
class  ScreenshotWriter;
class  SoundManager;
class  Sun;
class  TcpClient;
class  TrafficLightsHandler;
class  UpdateViewerHandler;
class  VehiclesHandler;

namespace vsg
{

class AmbientLight;
class Camera;
class CommandGraph;
class Group;
class InstanceNode;
class LookAt;
class Options;
class RegionOfInterest;
class ShaderSet;
class ShadowSettings;
class View;
class Viewer;
class Window;
class WindowTraits;

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
class RouteViewer final : public QObject
{
    Q_OBJECT

public:
    explicit RouteViewer(QObject* parent = nullptr);
    ~RouteViewer();

    void initialize(int argc, char* argv[]);

    int run();

private:
    void loadSettings();
    void loadNetworkSettings(CfgReader& cfg, const QString& section);
    void loadLoggerSettings(CfgReader& cfg, const QString& section);
    void loadModelsSettings(CfgReader& cfg, const QString& section);
    void loadWindowSettings(CfgReader& cfg, const QString& section);
    void loadLightSettings(CfgReader& cfg, const QString& section);
    void loadCameraSettings(CfgReader& cfg, const QString& section);
    void loadFreeCameraSettings(CfgReader& cfg, const QString& section);
    void loadCabineCameraSettings(CfgReader& cfg, const QString& section);
    void loadExternalCameraSettings(CfgReader& cfg, const QString& section);
    void loadFollowCameraSettings(CfgReader& cfg, const QString& section);

    void configureLogLevel() const;

    void overrideSettingsByCommandLine(int argc, char* argv[]);

    void initVsgOptions();
    void initWindowTraits();
    void initWindow(bool try_screenNum_exception = true);
    void initCamera();
    void initScenegraph();

    void initLights();
    void configureShaders();
    void loadCustomShader(
        FileSystem& fs,
        const std::string& shaders_dir_path,
        const char* vert_shader_filename,
        const char* frag_shader_filename,
        const char* shader_set_name,
        vsg::ref_ptr<vsg::ShaderSet> shader_set
    );

    void initView();
    void initCommandGraph();
    void initViewer();

    void initTcpClient();

    bool loadRoute();

private slots:
    void slotRecvLogMessage(QString msg);

    void slotConnectedToSimulator();

    void slotGetRouteInfoData(QByteArray &data);

    void slotGetSignalsData(QByteArray &sig_data);

    void slotGetVehicleInfoData(QByteArray &data);

    void slotUpdated();

private:
    bool  is_ready = false;
    bool  is_connection_abandoned = false;
    bool  is_route = false;
    bool  is_signals = false;
    bool  is_vehicles = false;

    settings_t settings;

    vsg::ref_ptr<GUIParams>            GUIparams;
    vsg::ref_ptr<UpdateViewerHandler>  upd_viewer_handler;

    std::unique_ptr<TcpClient>             tcp_client;
    std::unique_ptr<SoundManager>          sound_manager;
    std::unique_ptr<ScreenshotWriter>      screenshot_writer;
    std::unique_ptr<TrafficLightsHandler>  traffic_lights_handler;
    std::unique_ptr<VehiclesHandler>       vehicles_handler;
    std::unique_ptr<NewSkybox>             skybox;

    vsg::ref_ptr<vsg::Options>       options;
    vsg::ref_ptr<vsg::WindowTraits>  windowTraits;
    vsg::ref_ptr<vsg::Window>        window;
    vsg::ref_ptr<vsg::LookAt>        lookAt;
    vsg::ref_ptr<vsg::Camera>        camera;
    vsg::ref_ptr<vsg::View>          view;
    vsg::ref_ptr<vsg::CommandGraph>  commandGraph;
    vsg::ref_ptr<vsg::Viewer>        viewer;

    // Pending InstanceNodes from background loading threads
    std::mutex                                          pending_nodes_mutex;
    std::vector<vsg::ref_ptr<vsg::Node>>                pending_nodes;
    vsg::ref_ptr<vsg::Group>                            route_root;

    // Per-cell instance data for CPU-side frustum culling. Each cell stores
    // the master (full) transforms + model bounding radius; each frame we
    // rewrite the InstanceNode's vec3/quatArray with only the visible subset
    // and update instanceCount so vkCmdDrawIndexed skips culled instances.
    struct InstanceCell {
        vsg::ref_ptr<vsg::InstanceNode>       instance_node;
        vsg::dsphere                          cell_bound;
        std::vector<vsg::vec3>                master_translations;
        std::vector<vsg::quat>                master_rotations;
        std::vector<vsg::vec3>                master_scales;
        float                                 model_radius = 0.0f;
        // Cached last-frame visibility state to skip redundant buffer uploads
        // when the visible subset is unchanged.
        uint32_t                              last_visible_count = 0;
        uint32_t                              last_hash = 0;
    };
    std::mutex                                pending_cells_mutex;
    std::vector<std::shared_ptr<InstanceCell>> pending_cells;
    std::vector<std::shared_ptr<InstanceCell>> instance_cells;

    void cullInstanceCells();

    vsg::ref_ptr<vsg::Group>             root;
    vsg::ref_ptr<vsg::ShadowSettings>    shadowSettings;
    vsg::ref_ptr<vsg::RegionOfInterest>  shadow_region;
    vsg::ref_ptr<Sun>                    sun;
};

#endif // ROUTE_VIEWER_H
