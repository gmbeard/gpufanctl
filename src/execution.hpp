#ifndef GPUFANCTL_EXECUTION_HPP_INCLUDED
#define GPUFANCTL_EXECUTION_HPP_INCLUDED

/* GENERAL NOTES:
 * - OperationState shouldn't be moveable, only constructible and desctructible
 * - Receivers shouldn't hold operation state. This makes the above point hard
 *   to enforce when they're passed down the `connect()` chain. This
 *   relationship should instead be modelled by returning a new operation state
 *   containing the receiver itself, and do a defered connect / start.
 */

#include "execution/assertion.hpp"
#include "execution/box.hpp"
#include "execution/connect.hpp"
#include "execution/defer.hpp"
#include "execution/get_stop_token.hpp"
#include "execution/inline_delay_scheduler.hpp"
#include "execution/inline_signal_scheduler.hpp"
#include "execution/just_from.hpp"
#include "execution/repeat_effect.hpp"
#include "execution/schedule.hpp"
#include "execution/single_thread_context.hpp"
#include "execution/start.hpp"
#include "execution/stop_when.hpp"
#include "execution/sync_wait.hpp"
#include "execution/then.hpp"

#endif
