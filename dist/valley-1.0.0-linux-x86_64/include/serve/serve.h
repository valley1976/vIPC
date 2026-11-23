// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// serve 的伞形头文件：只包含公开门面与基础设施。
// TCP / IPC / RPC 的实现头文件在 src/ 下，不再对外暴露。

#include "serve/address.h"
#include "serve/client.h"
#include "serve/event_loop.h"
#include "serve/message.h"
#include "serve/server.h"
#include "serve/session_base.h"
#include "serve/status.h"
#include "serve/steady_timer.h"
#include "serve/types.h"
