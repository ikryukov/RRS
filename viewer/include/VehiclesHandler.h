#pragma once
#ifndef VEHICLES_HANDLER_H
#define VEHICLES_HANDLER_H

#include <simulator-info-struct.h>
#include <simulator-update-struct.h>
#include <autopilot-timetable.h>
#include <VehicleExterior.h>

#include <vsg/core/ref_ptr.h>
#include <vsg/nodes/Group.h>

#include <QObject>
#include <QString>

#include <array>
#include <vector>

struct settings_t;
class SoundManager;

class QByteArray;

namespace vsg
{
    class Options;
    class Viewer;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
class VehiclesHandler final : public QObject
{
    Q_OBJECT

public:
    VehiclesHandler(const settings_t& settings, SoundManager* sound_manager, QObject* parent = nullptr);

    /// Get scene group
    vsg::ref_ptr<vsg::Group> getExterior() const noexcept;

    /// Info about current vehicle exterior
    VehicleExterior* getCurrentVehicle();
    int getCurrentVehicleIndex() const noexcept;
    int getControlledVehicleIndex() const noexcept;
    int getCurrentTrainIndex() const noexcept;

    bool isUpdated() const noexcept;

    simulator_time_t* getDateTime();

    QString getDebugMessage() const noexcept;

    void step(double t, double dt);

    bool selectNextTrain() noexcept;
    bool selectPrevTrain() noexcept;
    bool selectNextVehicle() noexcept;
    bool selectPrevVehicle() noexcept;

    bool selectControlVehicle() noexcept;
    bool returnToControlledVehicle() noexcept;

    bool load(
        QByteArray& data,
        const settings_t& settings,
        vsg::ref_ptr<vsg::Options> options
    );

    void set_camera_pos(const vsg::dvec3* camera_pos) noexcept { this->camera_pos = camera_pos; }

    /// Получить данные о графике движения, если таковые имеются в текущей ПЕ
    autopilot_timetable_t getTimetable()
    {
        autopilot_timetable_t timetable;

        if (cur_vehicle >= 0
            && static_cast<size_t>(cur_vehicle) < state_front.vehicles.size()
            && !state_front.vehicles[cur_vehicle].timetableData.isEmpty())
        {
            timetable.deserialize(state_front.vehicles[cur_vehicle].timetableData);
        }

        return timetable;
    }

public slots:
    void slotGetTrainsData(QByteArray& data);
    void slotGetVehiclesPosData(QByteArray& data);
    void slotGetVehiclesStateData(QByteArray& data);
    void slotGetVehicleControlled(QByteArray& data);

signals:
    void updated();

private:
    void updateDebugString();

    /// Advance interpolation read head when client_time catches up
    void advanceInterpolation(double client_time);

private:
    SoundManager* sound_manager;
    const vsg::dvec3* camera_pos = nullptr;

    /// Position data ring buffer — single-threaded (all via QueuedConnection)
    static constexpr size_t POS_BUF_SIZE = 5;
    std::array<simulator_update_pos_t, POS_BUF_SIZE> pos_buf;
    size_t pos_write = 0;      ///< Next write slot (wraps around POS_BUF_SIZE)
    size_t pos_count = 0;      ///< Total frames received (saturates at POS_BUF_SIZE)
    size_t pos_read = 0;       ///< Current interpolation target index
    size_t pos_read_prev = 0;  ///< Previous frame for interpolation

    double ref_time = 0.0;
    double time_difference = 0.0;
    double settings_delay = 0.17;

    /// Data about trains, received from server
    simulator_trains_update_t update_trains;

    /// Vehicle state double buffer
    simulator_vehicles_update_t state_front;
    simulator_vehicles_update_t state_back;
    bool is_new_state = false;

    /// Data about vehicles, received from server
    simulator_vehicles_info_t vehicles_info;

    /// Debug strings for controlled and current vehicles
    simulator_vehicle_controlled_update_t vehicle_controlled;

    /// Updated status
    bool is_updated = false;

    /// Vehicle number which is a referenced for camera
    int cur_vehicle = 0;

    /// Vehicle number which is contorolled by user
    int controlled_vehicle = 0;

    /// Debug message for current and controlled vehicles from server
    QString debug_message;

    /// Train exterior scene group
    vsg::ref_ptr<vsg::Group> vehicles_node = vsg::Group::create();

    /// Info about vehicles exterior
    std::vector<VehicleExterior> vehicles;
};

#endif // VEHICLES_HANDLER_H
