#include "rkaiq_protocol.h"

#include <signal.h>
#include <sys/types.h>

#ifdef __ANDROID__
#include <cutils/properties.h>
#endif

#include "domain_tcp_client.h"
#include "tcp_client.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "aiqtool"

extern int g_app_run_mode;
extern int g_width;
extern int g_height;
extern int g_rtsp_en;
extern int g_device_id;
extern int g_allow_killapp;
extern DomainTCPClient g_tcpClient;
extern struct ucred* g_aiqCred;
extern std::string iqfile;
extern std::string g_sensor_name;
extern std::shared_ptr<RKAiqMedia> rkaiq_media;

bool RKAiqProtocol::is_recv_running = false;
std::shared_ptr<std::thread> RKAiqProtocol::forward_thread = nullptr;
std::mutex RKAiqProtocol::mutex_;

static int ProcessExists(const char* process_name) {
  FILE* fp;
  char cmd[1024] = {0};
  char buf[1024] = {0};
  snprintf(cmd, sizeof(cmd), "ps -ef | grep %s | grep -v grep", process_name);
  fp = popen(cmd, "r");
  if (!fp) {
    LOG_INFO("popen ps | grep %s fail\n", process_name);
    return -1;
  }
  while (fgets(buf, sizeof(buf), fp)) {
    LOG_INFO("ProcessExists %s\n", buf);
    if (strstr(buf, process_name)) {
      fclose(fp);
      return 1;
    }
  }
  fclose(fp);
  return 0;
}

int StopProcess(const char* process, const char* str) {
  int count = 0;
  while (ProcessExists(process) > 0) {
    LOG_INFO("StopProcess %s... \n", process);
    system(str);
    sleep(1);
    count++;
    if (count > 3) {
      return -1;
    }
  }
  return 0;
}

int WaitProcessExit(const char* process, int sec) {
  int count = 0;
  LOG_INFO("WaitProcessExit %s... \n", process);
  while (ProcessExists(process) > 0) {
    LOG_INFO("WaitProcessExit %s... \n", process);
    sleep(1);
    count++;
    if (count > sec) {
      return -1;
    }
  }
  return 0;
}

int RKAiqProtocol::DoChangeAppMode(appRunStatus mode) {
  int ret = -1;
  LOG_DEBUG("Switch to mode %d->%d\n", g_app_run_mode, mode);
  if (g_app_run_mode == mode) {
    return 0;
  }
  if (mode == APP_RUN_STATUS_STREAMING) {
    LOG_DEBUG("Switch to APP_RUN_STATUS_STREAMING\n");
    ret = rkaiq_media->LinkToIsp(true);
    if (ret) {
      LOG_ERROR("Switch mode failed!!!");
      return ret;
    }
#ifdef __ANDROID__
    if (g_allow_killapp) {
      property_set("ctrl.start", "cameraserver");
      system("start cameraserver");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (g_tcpClient.Setup(LOCAL_SOCKET_PATH) == false) {
      LOG_DEBUG("domain connect failed\n");
    }
#else
    if (g_device_id == 0) {
      if (g_tcpClient.Setup("/tmp/UNIX.domain") == false) {
        LOG_DEBUG("domain connect failed\n");
      }
    } else {
      if (g_tcpClient.Setup("/tmp/UNIX_1.domain") == false) {
        LOG_DEBUG("domain connect failed\n");
      }
    }
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#ifndef __ANDROID__
    if (g_rtsp_en) {
      media_info_t mi = rkaiq_media->GetMediaInfoT(g_device_id);
      int isp_ver = rkaiq_media->GetIspVer();
      LOG_DEBUG(">>>>>>>> isp ver = %d\n", isp_ver);
      if (isp_ver == 4) {
        ret = init_rtsp(mi.ispp.pp_scale0_path.c_str(), g_width, g_height);
      } else if (isp_ver == 5) {
        ret = init_rtsp(mi.isp.main_path.c_str(), g_width, g_height);
      } else {
        ret = init_rtsp(mi.isp.main_path.c_str(), g_width, g_height);
      }
      if (ret) {
        LOGE_ERROR("init_rtsp failed!!");
        return ret;
      }
    }
#endif
  } else if (mode == APP_RUN_STATUS_CAPTURE) {
    LOG_DEBUG("Switch to APP_RUN_STATUS_CAPTURE\n");
#ifndef __ANDROID__
    if (g_rtsp_en) {
      deinit_rtsp();
    }
#endif
#ifdef __ANDROID__
    if (g_allow_killapp) {
      unlink(LOCAL_SOCKET_PATH);
      property_set("ctrl.stop", "cameraserver");
      property_set("ctrl.stop", "vendor.camera-provider-2-4");
      property_set("ctrl.stop", "vendor.camera-provider-2-4-ext");
      system("stop cameraserver");
      system("stop vendor.camera-provider-2-4");
      system("stop vendor.camera-provider-2-4-ext");
    }
#else
    if (g_allow_killapp) {
      if (g_aiqCred != nullptr) {
        kill(g_aiqCred->pid, SIGTERM);
        delete g_aiqCred;
        g_aiqCred = nullptr;
      }
    }
#endif
#if 0
    rkaiq_manager.reset();
    rkaiq_manager = nullptr;
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ret = rkaiq_media->LinkToIsp(false);
    if (ret) {
      LOG_ERROR("unlink isp failed!!!");
      return ret;
    }
  } else {
    LOG_DEBUG("Switch to APP_RUN_STATUS_TUNRING\n");
#ifndef __ANDROID__
    if (g_rtsp_en) {
      deinit_rtsp();
    }
#endif
    ret = rkaiq_media->LinkToIsp(true);
    if (ret) {
      LOG_ERROR("link isp failed!!!");
      return ret;
    }
#ifdef __ANDROID__
    if (g_allow_killapp) {
      property_set("ctrl.start", "cameraserver");
      system("start cameraserver");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (g_tcpClient.Setup(LOCAL_SOCKET_PATH) == false) {
      LOG_ERROR("domain connect failed\n");
      return -1;
    }
#else
    if (g_device_id == 0) {
      if (g_tcpClient.Setup("/tmp/UNIX.domain") == false) {
        LOG_ERROR("domain connect failed\n");
        return -1;
      }
    } else {
      if (g_tcpClient.Setup("/tmp/UNIX_1.domain") == false) {
        LOG_ERROR("domain connect failed\n");
        return -1;
      }
    }
#endif

#if 0
    rkaiq_manager = std::make_shared<RKAiqToolManager>();
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  g_app_run_mode = mode;
  LOG_DEBUG("CHange mode to %d exit\n", g_app_run_mode);
  return 0;
}

static void InitCommandPingAns(CommandData_t* cmd, int ret_status) {
  strncpy((char*)cmd->RKID, RKID_CHECK, sizeof(cmd->RKID));
  cmd->cmdType = DEVICE_TO_PC;
  cmd->cmdID = CMD_ID_CAPTURE_STATUS;
  cmd->datLen = 1;
  memset(cmd->dat, 0, sizeof(cmd->dat));
  cmd->dat[0] = ret_status;
  cmd->checkSum = 0;
  for (int i = 0; i < cmd->datLen; i++) {
    cmd->checkSum += cmd->dat[i];
  }
}

static void DoAnswer(int sockfd, CommandData_t* cmd, int cmd_id, int ret_status) {
  char send_data[MAXPACKETSIZE];
  LOG_INFO("enter\n");

  strncpy((char*)cmd->RKID, TAG_OL_DEVICE_TO_PC, sizeof(cmd->RKID));
  cmd->cmdType = DEVICE_TO_PC;
  cmd->cmdID = cmd_id;
  strncpy((char*)cmd->version, RKAIQ_TOOL_VERSION, sizeof(cmd->version));
  cmd->datLen = 4;
  memset(cmd->dat, 0, sizeof(cmd->dat));
  cmd->dat[0] = ret_status;
  cmd->checkSum = 0;
  for (int i = 0; i < cmd->datLen; i++) {
    cmd->checkSum += cmd->dat[i];
  }

  memcpy(send_data, cmd, sizeof(CommandData_t));
  send(sockfd, send_data, sizeof(CommandData_t), 0);
  LOG_INFO("exit\n");
}

void RKAiqProtocol::HandlerCheckDevice(int sockfd, char* buffer, int size) {
  CommandData_t* common_cmd = (CommandData_t*)buffer;
  CommandData_t send_cmd;
  char send_data[MAXPACKETSIZE];

  LOG_INFO("HandlerRawCapMessage:\n");

  // for (int i = 0; i < common_cmd->datLen; i++) {
  //   LOG_INFO("DATA[%d]: 0x%x\n", i, common_cmd->dat[i]);
  // }

  if (strcmp((char*)common_cmd->RKID, RKID_CHECK) == 0) {
    LOG_INFO("RKID: %s\n", common_cmd->RKID);
  } else {
    LOG_INFO("RKID: Unknow\n");
    return;
  }

  LOG_INFO("cmdID: %d\n", common_cmd->cmdID);

  switch (common_cmd->cmdID) {
    case CMD_ID_CAPTURE_STATUS:
      LOG_INFO("CmdID CMD_ID_CAPTURE_STATUS in\n");
      if (common_cmd->dat[0] == KNOCK_KNOCK) {
        InitCommandPingAns(&send_cmd, READY);
        LOG_INFO("Device is READY\n");
      } else {
        LOG_ERROR("Unknow CMD_ID_CAPTURE_STATUS message\n");
      }
      memcpy(send_data, &send_cmd, sizeof(CommandData_t));
      send(sockfd, send_data, sizeof(CommandData_t), 0);
      LOG_INFO("cmdID CMD_ID_CAPTURE_STATUS out\n\n");
      break;
    case CMD_ID_GET_STATUS:
      DoAnswer(sockfd, &send_cmd, common_cmd->cmdID, READY);
      break;
    default:
      break;
  }
}

void RKAiqProtocol::HandlerTCPMessage(int sockfd, char* buffer, int size) {
  std::lock_guard<std::mutex> lg(mutex_);
  CommandData_t* common_cmd = (CommandData_t*)buffer;
  LOG_INFO("HandlerTCPMessage:\n");
  LOG_INFO("HandlerTCPMessage CommandData_t: 0x%x\n", sizeof(CommandData_t));
  LOG_INFO("HandlerTCPMessage RKID: %s\n", (char*)common_cmd->RKID);

  // TODO Check APP Mode

  if (strcmp((char*)common_cmd->RKID, TAG_PC_TO_DEVICE) == 0) {
    RKAiqRawProtocol::HandlerRawCapMessage(sockfd, buffer, size);
  } else if (strcmp((char*)common_cmd->RKID, TAG_OL_PC_TO_DEVICE) == 0) {
    RKAiqOLProtocol::HandlerOnLineMessage(sockfd, buffer, size);
  } else if (strcmp((char*)common_cmd->RKID, RKID_CHECK) == 0) {
    HandlerCheckDevice(sockfd, buffer, size);
  } else {
    DoChangeAppMode(APP_RUN_STATUS_TUNRING);
    MessageForward(sockfd, buffer, size);
  }
}

int RKAiqProtocol::doMessageForward(int sockfd) {
  is_recv_running = true;
  while (is_recv_running) {
    char recv_buffer[MAXPACKETSIZE] = {0};
    int recv_len = g_tcpClient.Receive(recv_buffer, MAXPACKETSIZE);
    if (recv_len > 0) {
      send(sockfd, recv_buffer, recv_len, 0);
    }
  }

  return 0;
}

int RKAiqProtocol::MessageForward(int sockfd, char* buffer, int size) {
  LOG_INFO("[%s]got data:%d!\n", __func__, size);
  g_tcpClient.Send((char*)buffer, size);

  if (is_recv_running) {
    return 0;
  }

  forward_thread = make_shared<thread>(&RKAiqProtocol::doMessageForward, sockfd);

  return 0;
}
