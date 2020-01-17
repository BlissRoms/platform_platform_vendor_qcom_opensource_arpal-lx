/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef STREAMSOUNDTRIGGER_H_
#define STREAMSOUNDTRIGGER_H_

#include <utility>
#include <map>

#include "Stream.h"
#include "SoundTriggerEngine.h"
#include "QalRingBuffer.h"

/* Event Mode
 * Indicating info GECKO will notify to client
 */
#define CONFIDENCE_LEVEL_INFO    0x1
#define KEYWORD_INDICES_INFO     0x2
#define TIME_STAMP_INFO          0x4
#define FTRT_INFO                0x8

enum {
    ENGINE_IDLE  = 0x0,
    GMM_DETECTED = 0x1,
    CNN_DETECTED = 0x2,
    CNN_REJECTED = 0x4,
    VOP_DETECTED = 0x8,
    VOP_REJECTED = 0x10,
};

enum {
    ST_STATE_NONE,
    ST_STATE_IDLE,
    ST_STATE_LOADED,
    ST_STATE_ACTIVE,
    ST_STATE_DETECTED,
    ST_STATE_BUFFERING,
};

enum {
    ST_EV_LOAD_SOUND_MODEL,
    ST_EV_UNLOAD_SOUND_MODEL,
    ST_EV_RECOGNITION_CONFIG,
    ST_EV_START_RECOGNITION,
    ST_EV_RESTART_RECOGNITION,
    ST_EV_STOP_RECOGNITION,
    ST_EV_DETECTED,
    ST_EV_READ_BUFFER,
    ST_EV_STOP_BUFFERING,
    ST_EV_STOP_DEVICE_CONNECTED,
    ST_EV_STOP_DEVICE_DISCONNECTED,
    ST_EV_STOP_SSR_OFFLINE,
    ST_EV_STOP_SSR_ONLINE,
    ST_EV_CONCURRENT_STREAM
};

class ResourceManager;
class SoundTriggerEngine;

class StreamSoundTrigger : public Stream {
 public:
    StreamSoundTrigger(struct qal_stream_attributes *sattr,
                       struct qal_device *dattr,
                       uint32_t no_of_devices,
                       struct modifier_kv *modifiers __unused,
                       uint32_t no_of_modifiers __unused,
                       std::shared_ptr<ResourceManager> rm);
    ~StreamSoundTrigger();
    int32_t open() { return 0; }

    int32_t close() override;
    int32_t start() override;
    int32_t stop() override;

    int32_t prepare() override { return 0; }

    int32_t setStreamAttributes(struct qal_stream_attributes *sattr __unused) {
        return 0;
    }

    int32_t setVolume(struct qal_volume_data * volume __unused) { return 0; }
    int32_t setMute(bool state __unused) override { return 0; }
    int32_t setPause() override { return 0; }
    int32_t setResume() override { return 0; }

    int32_t read(struct qal_buffer *buf) override;

    int32_t write(struct qal_buffer *buf __unused) { return 0; }

    int32_t registerCallBack(qal_stream_callback cb,  void *cookie) override;
    int32_t getCallBack(qal_stream_callback *cb) override;
    int32_t getParameters(uint32_t param_id, void **payload) override;
    int32_t setParameters(uint32_t param_id, void *payload) override;

    int32_t addRemoveEffect(qal_audio_effect_t effect __unused,
                            bool enable __unused) {
        return -ENOSYS;
    }

    struct detection_event_info* GetDetectionEventInfo();
    int32_t ParseDetectionPayload(uint32_t *event_data);
    void SetDetectedToEngines(bool detected);
    int32_t SetEngineDetectionState(int32_t state);
    int32_t notifyClient();

    static int32_t isSampleRateSupported(uint32_t sampleRate);
    static int32_t isChannelSupported(uint32_t numChannels);
    static int32_t isBitWidthSupported(uint32_t bitWidth);

    int switchDevice(Stream* stream_handle, uint32_t no_of_devices,
                     struct qal_device *device_array);
    void ConcurrentStreamStatus(int stream_type, bool is_active) override;
    void TransitTo(int32_t state_id);

    friend class QalRingBufferReader;

 private:
    class EngineCfg {
     public:
        EngineCfg(int32_t id, std::shared_ptr<SoundTriggerEngine> engine,
                  void *data, int32_t size)
            : id_(id), engine_(engine), sm_data_(data), sm_size_(size) {}

        ~EngineCfg() {}

        std::shared_ptr<SoundTriggerEngine> GetEngine() const {
            return engine_;
        }
        int32_t GetEngineId() const { return id_; }

        int32_t id_;
        std::shared_ptr<SoundTriggerEngine> engine_;
        void *sm_data_;
        int32_t sm_size_;
    };

    class StEventConfigData {
     public:
        StEventConfigData() {}
        virtual ~StEventConfigData() {}
    };

    class StEventConfig {
     public:
        explicit StEventConfig(int32_t ev_id)
            : id_(ev_id), data_(nullptr) {}
        virtual ~StEventConfig() {}

        int32_t id_; // event id
        std::shared_ptr<StEventConfigData> data_; // event specific data
    };

    class StLoadEventConfigData : public StEventConfigData {
     public:
        StLoadEventConfigData(void *data): data_(data) {}
        ~StLoadEventConfigData() {}

        void *data_;
    };

    class StLoadEventConfig : public StEventConfig {
     public:
        StLoadEventConfig(void *data)
            : StEventConfig(ST_EV_LOAD_SOUND_MODEL) {
           data_ = std::make_shared<StLoadEventConfigData>(data);
        }
        ~StLoadEventConfig() {}
    };

    class StUnloadEventConfig : public StEventConfig {
     public:
        StUnloadEventConfig() : StEventConfig(ST_EV_UNLOAD_SOUND_MODEL) {}
        ~StUnloadEventConfig() {}
    };

    class StRecognitionCfgEventConfigData : public StEventConfigData {
     public:
        StRecognitionCfgEventConfigData(void *data): data_(data) {}
        ~StRecognitionCfgEventConfigData() {}

        void *data_;
    };

    class StRecognitionCfgEventConfig : public StEventConfig {
     public:
        StRecognitionCfgEventConfig(void *data)
            : StEventConfig(ST_EV_RECOGNITION_CONFIG) {
            data_ = std::make_shared<StRecognitionCfgEventConfigData>(data);
        }
        ~StRecognitionCfgEventConfig() {}
    };

    class StStartRecognitionEventConfigData : public StEventConfigData {
     public:
        StStartRecognitionEventConfigData(int32_t rs) : restart_(rs) {}
        ~StStartRecognitionEventConfigData() {}

        bool restart_;
    };

    class StStartRecognitionEventConfig : public StEventConfig {
     public:
        StStartRecognitionEventConfig(bool restart)
            : StEventConfig(ST_EV_START_RECOGNITION) {
            data_ = std::make_shared<StStartRecognitionEventConfigData>(restart);
        }
        ~StStartRecognitionEventConfig() {}
    };

    class StStopRecognitionEventConfigData : public StEventConfigData {
     public:
        StStopRecognitionEventConfigData(bool deferred) : deferred_(deferred) {}
        ~StStopRecognitionEventConfigData() {}

        bool deferred_;
    };

    class StStopRecognitionEventConfig : public StEventConfig {
     public:
        StStopRecognitionEventConfig(bool deferred)
            : StEventConfig(ST_EV_STOP_RECOGNITION) {
            data_ = std::make_shared<StStopRecognitionEventConfigData>(deferred);
        }
        ~StStopRecognitionEventConfig() {}
    };

    class StDetectedEventConfigData : public StEventConfigData {
     public:
        StDetectedEventConfigData(int32_t type) : det_type_(type) {}
        ~StDetectedEventConfigData() {}

        int32_t det_type_;
    };

    class StDetectedEventConfig : public StEventConfig {
     public:
        StDetectedEventConfig(int32_t type) : StEventConfig(ST_EV_DETECTED) {
            data_ = std::make_shared<StDetectedEventConfigData>(type);
        }
        ~StDetectedEventConfig() {}
    };

    class StReadBufferEventConfigData : public StEventConfigData {
     public:
        StReadBufferEventConfigData(void *data) : data_(data) {}
        ~StReadBufferEventConfigData() {}

        void *data_;
    };

    class StReadBufferEventConfig : public StEventConfig {
     public:
        StReadBufferEventConfig(void *data) : StEventConfig(ST_EV_READ_BUFFER) {
            data_ = std::make_shared<StReadBufferEventConfigData>(data);
        }
        ~StReadBufferEventConfig() {}
    };

    class StStopBufferingEventConfig : public StEventConfig {
     public:
        StStopBufferingEventConfig () : StEventConfig(ST_EV_STOP_BUFFERING) {}
        ~StStopBufferingEventConfig () {}
    };

    class StConcurrentStreamEventConfigData : public StEventConfigData {
     public:
        StConcurrentStreamEventConfigData(int32_t type, bool active)
            : stream_type_(type), is_active_(active) {}
        ~StConcurrentStreamEventConfigData() {}

        int32_t stream_type_;
        bool is_active_;
    };
    class StConcurrentStreamEventConfig : public StEventConfig {
     public:
        StConcurrentStreamEventConfig (int32_t type, bool active)
            : StEventConfig(ST_EV_CONCURRENT_STREAM) {
            data_ = std::make_shared<StConcurrentStreamEventConfigData>(type,
                                                                        active);
        }
        ~StConcurrentStreamEventConfig () {}
    };

    class StState {
     public:
        StState(StreamSoundTrigger& st_stream, int32_t state_id)
            : st_stream_(st_stream), state_id_(state_id) {}
        virtual ~StState() {}

        int32_t GetStateId() { return state_id_; }

     protected:
        virtual int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) = 0;

        void TransitTo(int32_t state_id) { st_stream_.TransitTo(state_id); }

     private:
        StreamSoundTrigger& st_stream_;
        int32_t state_id_;

        friend class StreamSoundTrigger;
    };

    class StIdle : public StState {
     public:
        StIdle(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_IDLE) {}
        ~StIdle() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    class StLoaded : public StState {
     public:
        StLoaded(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_LOADED) {}
        ~StLoaded() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    class StActive : public StState {
     public:
        StActive(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_ACTIVE) {}
        ~StActive() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    class StDetected : public StState {
     public:
        StDetected(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_DETECTED) {}
        ~StDetected() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    class StBuffering : public StState {
     public:
        StBuffering(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_BUFFERING) {}
        ~StBuffering() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    qal_device_id_t GetAvailCaptureDevice();
    void AddEngine(std::shared_ptr<EngineCfg> engine_cfg);
    int32_t LoadSoundModel(struct qal_st_sound_model *sm_data);
    int32_t SendRecognitionConfig(struct qal_st_recognition_config *config);
    bool compareRecognitionConfig(
       const struct qal_st_recognition_config *current_config,
       struct qal_st_recognition_config *new_config);

    int32_t ParseOpaqueConfLevels(void *opaque_conf_levels,
                                  uint32_t version,
                                  uint8_t **out_conf_levels,
                                  uint32_t *out_num_conf_levels);
    int32_t FillConfLevels(struct qal_st_recognition_config *config,
                           uint8_t **out_conf_levels,
                           uint32_t *out_num_conf_levels);
    int32_t FillOpaqueConfLevels(const void *sm_levels_generic,
                                 uint8_t **out_payload,
                                 uint32_t *out_payload_size,
                                 uint32_t version);
    int32_t GenerateCallbackEvent(struct qal_st_recognition_event **event);
    static int32_t HandleDetectionEvent(qal_stream_handle_t *stream_handle,
                                        uint32_t event_id,
                                        uint32_t *event_data,
                                        void *cookie __unused);

    static void TimerThread(StreamSoundTrigger& st_stream);
    void PostDelayedStop();
    void CancelDelayedStop();
    void InternalStopRecognition();
    std::thread timer_thread_;
    std::mutex timer_mutex_;
    std::condition_variable timer_start_cond_;
    std::condition_variable timer_wait_cond_;
    bool timer_stop_waiting_;
    bool exit_timer_thread_;
    bool pending_stop_;

    void AddState(StState* state);
    int32_t GetCurrentStateId();
    int32_t GetPreviousStateId();
    int32_t ProcessInternalEvent(std::shared_ptr<StEventConfig> ev_cfg);

    std::vector<std::shared_ptr<EngineCfg>> engines_;
    std::shared_ptr<SoundTriggerEngine> gsl_engine_;

    qal_st_sound_model_type_t sound_model_type_;
    struct qal_st_phrase_sound_model *sm_config_;
    struct qal_st_recognition_config *rec_config_;
    uint32_t detection_state_;
    uint32_t notification_state_;
    qal_stream_callback callback_;
    void * cookie_;
    QalRingBufferReader *reader_;

    StState *st_idle_;
    StState *st_loaded_;
    StState *st_active;
    StState *st_detected_;
    StState *st_buffering_;

    StState *cur_state_;
    StState *prev_state_;
    std::map<uint32_t, StState*> st_states_;
};
#endif // STREAMSOUNDTRIGGER_H_
