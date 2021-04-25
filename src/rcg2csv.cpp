// -*-c++-*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

#include <rcsc/param/param_map.h>
#include <rcsc/param/cmd_line_parser.h>

#include <rcsc/common/server_param.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/player_type.h>

#include <rcsc/types.h>
#include <rcsc/gz.h>
#include <rcsc/rcg.h>

class CSVPrinter
    : public rcsc::rcg::Handler {
private:
    struct CommandCount {
        int kick_;
        int dash_;
        int turn_;
        int say_;
        int turn_neck_;
        int catch_;
        int move_;
        int change_view_;

        CommandCount()
            : kick_( 0 )
            , dash_( 0 )
            , turn_( 0 )
            , say_( 0 )
            , turn_neck_( 0 )
            , catch_( 0 )
            , move_( 0 )
            , change_view_( 0 )
          { }
        void update( const rcsc::rcg::player_t & player )
          {
              kick_ = rcsc::rcg::nstohi( player.kick_count );
              dash_ = rcsc::rcg::nstohi( player.dash_count );
              turn_ = rcsc::rcg::nstohi( player.turn_count );
              say_ = rcsc::rcg::nstohi( player.say_count );
              turn_neck_ = rcsc::rcg::nstohi( player.turn_neck_count );
              catch_ = rcsc::rcg::nstohi( player.catch_count );
              move_ = rcsc::rcg::nstohi( player.move_count );
              change_view_ = rcsc::rcg::nstohi( player.change_view_count );
          }
        void update( const rcsc::rcg::PlayerT & player )
          {
              kick_ = player.kick_count_;
              dash_ = player.dash_count_;
              turn_ = player.turn_count_;
              say_ = player.say_count_;
              turn_neck_ = player.turn_neck_count_;
              catch_ = player.catch_count_;
              move_ = player.move_count_;
              change_view_ = player.change_view_count_;
          }
    };


    std::ostream & M_os;

    int M_show_count;

    rcsc::rcg::UInt32 M_cycle;
    rcsc::rcg::UInt32 M_stopped;

    rcsc::PlayMode M_playmode;
    rcsc::rcg::TeamT M_teams[2];

    CommandCount M_command_count[rcsc::MAX_PLAYER * 2];

    int playerTypesCount;

    // not used
    CSVPrinter();
public:

    explicit
    CSVPrinter( std::ostream & os );

    virtual
    bool handleLogVersion( const int ver );

    virtual
    bool handleEOF();

    virtual
    bool handleShow( const rcsc::rcg::ShowInfoT & show );
    virtual
    bool handleMsg( const int time,
                    const int board,
                    const std::string & msg );
    virtual
    bool handleDraw( const int time,
                     const rcsc::rcg::drawinfo_t & draw );
    virtual
    bool handlePlayMode( const int time,
                         const rcsc::PlayMode pm );
    virtual
    bool handleTeam( const int time,
                     const rcsc::rcg::TeamT & team_l,
                     const rcsc::rcg::TeamT & team_r );
    virtual
    bool handleServerParam( const std::string & msg );
    virtual
    bool handlePlayerParam( const std::string & msg );
    virtual
    bool handlePlayerType( const std::string & msg );

private:
    const std::string & getPlayModeString( const rcsc::PlayMode playmode ) const;

    std::ostream & printServerParam() const;
    std::ostream & printPlayerParam() const;

    std::ostream & printPlayerTypesHeader() const;
    std::ostream & printPlayerType(const rcsc::PlayerType& type) const;


    std::ostream & printShowHeader() const;
    std::ostream & printShowData( const rcsc::rcg::ShowInfoT & show ) const;

    // print values
    std::ostream & printShowCount() const;
    std::ostream & printTime() const;
    std::ostream & printPlayMode() const;
    std::ostream & printTeams() const;
    std::ostream & printBall( const rcsc::rcg::BallT & ball ) const;
    std::ostream & printPlayers( const rcsc::rcg::ShowInfoT & show ) const;
    std::ostream & printPlayer( const rcsc::rcg::PlayerT & player ) const;

};


/*-------------------------------------------------------------------*/
/*!

 */
CSVPrinter::CSVPrinter( std::ostream & os )
    : M_os( os ),
      M_show_count( 0 ),
      M_cycle( 0 ),
      M_stopped( 0 ),
      M_playmode( rcsc::PM_Null ),
      playerTypesCount( 0 )
{

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handleLogVersion( const int ver )
{
    rcsc::rcg::Handler::handleLogVersion( ver );

    if ( ver < 4 )
    {
        std::cerr << "Unsupported RCG version " << ver << std::endl;
        return false;
    }

    return true;
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handleEOF()
{
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handleShow( const rcsc::rcg::ShowInfoT & show )
{
    static bool s_first = true;
    if ( s_first )
    {
        printShowHeader();
        s_first = false;
    }

    // update show count
    ++M_show_count;

    // update game time
    if ( M_cycle == show.time_ )
    {
        ++M_stopped;
    }
    else
    {
        M_cycle = show.time_;
        M_stopped = 0;
    }

    printShowData( show );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handleMsg( const int,
                       const int,
                       const std::string & )
{
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handleDraw( const int,
                        const rcsc::rcg::drawinfo_t & )
{
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handlePlayMode( const int,
                            const rcsc::PlayMode pm )
{
    M_playmode = pm;

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handleTeam( const int,
                        const rcsc::rcg::TeamT & team_l,
                        const rcsc::rcg::TeamT & team_r )
{
    M_teams[0] = team_l;
    M_teams[1] = team_r;

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handleServerParam( const std::string & msg )
{
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        if ( ! rcsc::ServerParam::instance().parse( msg.c_str(), 8 ) ) {
            std::cerr << "ERROR: Failed to extract ServerParam object from server_param message." << std::endl;
            return false;
        }
        printServerParam();
        return true;
    }
    // There should be only one server_params log entry.
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handlePlayerParam( const std::string & msg )
{
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        if ( ! rcsc::PlayerParam::instance().parse( msg.c_str(), 8 ) ) {
            std::cerr << "ERROR: Failed to extract PlayerParam object from player_param message." << std::endl;
            return false;
        }
        printPlayerParam();
        return true;
    }
    // There should be only one player_param log entry.
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handlePlayerType( const std::string & msg )
{
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        printPlayerTypesHeader();
    }
    playerTypesCount++;
    printPlayerType(rcsc::PlayerType(msg.c_str(), 16.0)); // For game log version 5+ this should be at least 13.0
    // TODO: There's no way of checking parsing errors with the current implementation of rcsc::PlayerType
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
const std::string &
CSVPrinter::getPlayModeString( const rcsc::PlayMode playmode ) const
{
    static const std::string s_playmode_str[] = PLAYMODE_STRINGS;

    if ( playmode < rcsc::PM_Null
         || rcsc::PM_MAX < playmode )
    {
        return s_playmode_str[0];
    }

    return s_playmode_str[playmode];
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printServerParam() const
{
    const rcsc::ServerParam& sp = rcsc::ServerParam::i();
    //
    // Header
    //
    M_os    << "#" // Primary key
            << ",goal_width"
            << ",inertia_moment"

            << ",player_size"
            << ",player_decay"
            << ",player_rand"
            << ",player_weight"
            << ",player_speed_max"
            << ",player_accel_max"

            << ",stamina_max"
            << ",stamina_inc_max"

            << ",recover_init"
            << ",recover_dec_thr"
            << ",recover_min"
            << ",recover_dec"

            << ",effort_init"
            << ",effort_dec_thr"
            << ",effort_min"
            << ",effort_dec"
            << ",effort_inc_thr"
            << ",effort_inc"

            << ",kick_rand"
            << ",team_actuator_noise"
            << ",prand_factor_l"
            << ",prand_factor_r"
            << ",kick_rand_factor_l"
            << ",kick_rand_factor_r"

            << ",ball_size"
            << ",ball_decay"
            << ",ball_rand"
            << ",ball_weight"
            << ",ball_speed_max"
            << ",ball_accel_max"

            << ",dash_power_rate"
            << ",kick_power_rate"
            << ",kickable_margin"
            << ",control_radius"

            << ",maxpower"
            << ",minpower"
            << ",maxmoment"
            << ",minmoment"
            << ",maxneckmoment"
            << ",minneckmoment"
            << ",maxneckang"
            << ",minneckang"

            << ",visible_angle"
            << ",visible_distance"

            << ",wind_dir"
            << ",wind_force"
            << ",wind_ang"
            << ",wind_rand"

            << ",catchable_area_l"
            << ",catchable_area_w"
            << ",catch_probability"
            << ",goalie_max_moves"

            << ",ckick_margin"
            << ",offside_active_area_size"

            << ",wind_none"
            << ",wind_random"

            << ",say_coach_cnt_max"
            << ",say_coach_msg_size"

            << ",clang_win_size"
            << ",clang_define_win"
            << ",clang_meta_win"
            << ",clang_advice_win"
            << ",clang_info_win"
            << ",clang_mess_delay"
            << ",clang_mess_per_cycle"

            << ",half_time"
            << ",simulator_step"
            << ",send_step"
            << ",recv_step"
            << ",sense_body_step"

            << ",say_msg_size"
            << ",hear_max"
            << ",hear_inc"
            << ",hear_decay"

            << ",catch_ban_cycle"

            << ",slow_down_factor"

            << ",use_offside"
            << ",forbid_kick_off_offside"
            << ",offside_kick_margin"

            << ",audio_cut_dist"

            << ",quantize_step"
            << ",quantize_step_l"


            << ",coach"
            << ",coach_w_referee"
            << ",old_coach_hear"

            << ",slowness_on_top_for_left_team"
            << ",slowness_on_top_for_right_team"

            << ",start_goal_l"
            << ",start_goal_r"

            << ",fullstate_l"
            << ",fullstate_r"

            << ",drop_ball_time"

            << ",synch_mode"
            << ",synch_offset"
            << ",synch_micro_sleep"

            << ",point_to_ban"
            << ",point_to_duration"

            // not defined in server_params_t
            << ",port"
            << ",coach_port"
            << ",olcoach_port"

            << ",verbose"

            << ",send_vi_step"

            << ",landmark_file"

            << ",send_comms"

            // logging params are not used in normal client
            << ",text_logging"
            << ",game_logging"
            << ",game_log_version"
            << ",text_log_dir"
            << ",game_log_dir"
            << ",text_log_fixed_name"
            << ",game_log_fixed_name"
            << ",text_log_fixed"
            << ",game_log_fixed"
            << ",text_log_dated"
            << ",game_log_dated"
            << ",log_date_format"
            << ",log_times"
            << ",record_messages"
            << ",text_log_compression"
            << ",game_log_compression"
            << ",profile"


            << ",tackle_dist"
            << ",tackle_back_dist"
            << ",tackle_width"
            << ",tackle_exponent"
            << ",tackle_cycles"
            << ",tackle_power_rate"

            << ",freeform_wait_period"
            << ",freeform_send_period"

            << ",free_kick_faults"
            << ",back_passes"

            << ",proper_goal_kicks"
            << ",stopped_ball_vel"
            << ",max_goal_kicks"

            << ",clang_del_win"
            << ",clang_rule_win"

            << ",auto_mode"
            << ",kick_off_wait"
            << ",connect_wait"
            << ",game_over_wait"
            << ",team_l_start"
            << ",team_r_start"

            << ",keepaway"
            << ",keepaway_length"
            << ",keepaway_width"

            // logging params are not used in normal client
            << ",keepaway_logging"
            << ",keepaway_log_dir"
            << ",keepaway_log_fixed_name"
            << ",keepaway_log_fixed"
            << ",keepaway_log_dated"

            << ",keepaway_start"

            << ",nr_normal_halfs"
            << ",nr_extra_halfs"
            << ",penalty_shoot_outs"

            << ",pen_before_setup_wait"
            << ",pen_setup_wait"
            << ",pen_ready_wait"
            << ",pen_taken_wait"
            << ",pen_nr_kicks"
            << ",pen_max_extra_kicks"
            << ",pen_dist_x"
            << ",pen_random_winner"
            << ",pen_max_goalie_dist_x"
            << ",pen_allow_mult_kicks"
            << ",pen_coach_moves_players"


            << ",ball_stuck_area"
            << ",coach_msg_file"
            // 12
            << ",max_tackle_power"
            << ",max_back_tackle_power"
            << ",player_speed_max_min"
            << ",extra_stamina"
            << ",synch_see_offset"
            << ",max_monitors"
            // 12.1.3
            << ",extra_half_time"
            // 13.0.0
            << ",stamina_capacity"
            << ",max_dash_angle"
            << ",min_dash_angle"
            << ",dash_angle_step"
            << ",side_dash_rate"
            << ",back_dash_rate"
            << ",max_dash_power"
            << ",min_dash_power"
            // 14.0.0
            << ",tackle_rand_factor"
            << ",foul_detect_probability"
            << ",foul_exponent"
            << ",foul_cycles"
            << ",golden_goal"
            // 15.0.0
            << ",red_card_probability"
            // 16.0.0
            << ",illegal_defense_duration"
            << ",illegal_defense_number"
            << ",illegal_defense_dist_x"
            << ",illegal_defense_width"
            << ",fixed_teamname_l"
            << ",fixed_teamname_r"
            ;
    // Line break
    M_os << std::endl;
    //
    // Parameters values
    //
    M_os    << "1" // Row number
            << "," << sp.goalWidth()
            << "," << sp.defaultInertiaMoment()

            << "," << sp.defaultPlayerSize()
            << "," << sp.defaultPlayerDecay()
            << "," << sp.playerRand()
            << "," << sp.playerWeight()
            << "," << sp.defaultPlayerSpeedMax()
            << "," << sp.playerAccelMax()

            << "," << sp.staminaMax()
            << "," << sp.defaultStaminaIncMax()

            << "," << sp.recoverInit()
            << "," << sp.recoverDecThr()
            << "," << sp.recoverMin()
            << "," << sp.recoverDec()

            << "," << sp.effortInit()
            << "," << sp.effortDecThr()
            << "," << sp.defaultEffortMin()
            << "," << sp.effortDec()
            << "," << sp.effortIncThr()
            << "," << sp.effortIncThr()

            << "," << sp.defaultKickRand()
            << "," << sp.teamActuatorNoise()
            << "," << sp.playerRandFactorLeft()
            << "," << sp.playerRandFactorRight()
            << "," << sp.kickRandFactorLeft()
            << "," << sp.kickRandFactorRight()

            << "," << sp.ballSize()
            << "," << sp.ballDecay()
            << "," << sp.ballRand()
            << "," << sp.ballWeight()
            << "," << sp.ballSpeedMax()
            << "," << sp.ballAccelMax()

            << "," << sp.defaultDashPowerRate()
            << "," << sp.kickPowerRate()
            << "," << sp.defaultKickableMargin()
            << "," << sp.controlRadius()

            << "," << sp.maxPower()
            << "," << sp.minPower()
            << "," << sp.maxMoment()
            << "," << sp.minMoment()
            << "," << sp.maxNeckMoment()
            << "," << sp.minNeckMoment()
            << "," << sp.maxNeckAngle()
            << "," << sp.minNeckAngle()

            << "," << sp.visibleAngle()
            << "," << sp.visibleDistance()

            << "," << sp.windDir()
            << "," << sp.windForce()
            << "," << sp.windAngle()
            << "," << sp.windRand()

            << "," << sp.catchAreaLength()
            << "," << sp.catchAreaWidth()
            << "," << sp.catchProbability()
            << "," << sp.goalieMaxMoves()

            << "," << sp.cornerKickMargin()
            << "," << sp.offsideActiveAreaSize()

            << "," << sp.windNone()
            << "," << sp.windRand()

            << "," << sp.coachSayCountMax()
            << "," << sp.coachSayMsgSize()

            << "," << sp.clangWinSize()
            << "," << sp.clangDefineWin()
            << "," << sp.clangMetaWin()
            << "," << sp.clangAdviceWin()
            << "," << sp.clangInfoWin()
            << "," << sp.clangMessDelay()
            << "," << sp.clangMessPerCycle()

            << "," << sp.halfTime()
            << "," << sp.simulatorStep()
            << "," << sp.sendStep()
            << "," << sp.recvStep()
            << "," << sp.senseBodyStep()

            << "," << sp.playerSayMsgSize()
            << "," << sp.playerHearMax()
            << "," << sp.playerHearInc()
            << "," << sp.playerHearDecay()

            << "," << sp.catchBanCycle()

            << "," << sp.slowDownFactor()

            << "," << sp.useOffside()
            << "," << sp.kickoffOffside()
            << "," << sp.offsideKickMargin()

            << "," << sp.audioCutDist()

            << "," << sp.distQuantizeStep()
            << "," << sp.landmarkDistQuantizeStep()


            << "," << sp.coachMode()
            << "," << sp.coachWithRefereeMode()
            << "," << sp.useOldCoachHear()

            << "," << sp.slownessOnTopForLeft()
            << "," << sp.slownessOnTopForRight()

            << "," << sp.startGoalLeft()
            << "," << sp.stargGoalRight()

            << "," << sp.fullstateLeft()
            << "," << sp.fullstateRight()

            << "," << sp.dropBallTime()

            << "," << sp.synchMode()
            << "," << sp.synchOffset()
            << "," << sp.synchMicroSleep()

            << "," << sp.pointToBan()
            << "," << sp.pointToDuration()

            // not defined in server_params_t
            << "," << sp.playerPort()
            << "," << sp.trainerPort()
            << "," << sp.onlineCoachPort()

            << "," << sp.verboseMode()

            << "," << sp.coachSendVIStep()

            << "," << "\"" << sp.landmarkFile() << "\""

            << "," << sp.sendComms()

            // logging params are not used in normal client
            << "," << sp.textLogging()
            << "," << sp.gameLogging()
            << "," << sp.gameLogVersion()
            << "," << "\"" << sp.textLogDir() << "\""
            << "," << "\"" << sp.gameLogDir() << "\""
            << "," << "\"" << sp.textLogFixedName() << "\""
            << "," << "\"" << sp.gameLogFixedName() << "\""
            << "," << sp.textLogFixed()
            << "," << sp.gameLogFixed()
            << "," << sp.textLogDated()
            << "," << sp.gameLogDated()
            << "," << "\"" << sp.logDateFormat() << "\""
            << "," << sp.logTimes()
            << "," << sp.recordMessage()
            << "," << sp.textLogCompression()
            << "," << sp.gameLogCompression()
            << "," << sp.useProfile()


            << "," << sp.tackleDist()
            << "," << sp.tackleBackDist()
            << "," << sp.tackleWidth()
            << "," << sp.tackleExponent()
            << "," << sp.tackleCycles()
            << "," << sp.tacklePowerRate()

            << "," << sp.freeformWaitPeriod()
            << "," << sp.freeformSendPeriod()

            << "," << sp.freeKickFaults()
            << "," << sp.backPasses()

            << "," << sp.properGoalKicks()
            << "," << sp.stoppedBallVel()
            << "," << sp.maxGoalKicks()

            << "," << sp.clangDelWin()
            << "," << sp.clangRuleWin()

            << "," << sp.autoMode()
            << "," << sp.kickOffWait()
            << "," << sp.connectWait()
            << "," << sp.gameOverWait()
            << "," << "\"" << sp.teamLeftStartCommand() <<"\""
            << "," << "\"" << sp.teamRightStartCommand() << "\""

            << "," << sp.keepawayMode()
            << "," << sp.keepawayLength()
            << "," << sp.keepawayWidth()

            // logging params are not used in normal client
            << "," << sp.keepawayLogging()
            << "," << "\"" << sp.keepawayLogDir() << "\""
            << "," << "\"" << sp.keepawayLogFixedName() << "\""
            << "," << sp.keepawayLogFixed()
            << "," << sp.keepawayLogDated()

            << "," << sp.keepawayStart()

            << "," << sp.nrNormalHalfs()
            << "," << sp.nrExtraHalfs()
            << "," << sp.penaltyShootOuts()

            << "," << sp.penBeforeSetupWait()
            << "," << sp.penSetupWait()
            << "," << sp.penReadyWait()
            << "," << sp.penTakenWait()
            << "," << sp.penNrKicks()
            << "," << sp.penMaxExtraKicks()
            << "," << sp.penDistX()
            << "," << sp.penRandomWinner()
            << "," << sp.penMaxGoalieDistX()
            << "," << sp.penAllowMultKicks()
            << "," << sp.penCoachMovesPlayers()

            << "," << sp.ballStuckArea()
            << "," << "\"" << sp.coachMsgFile() << "\""
            // 12
            << "," << sp.maxTacklePower()
            << "," << sp.maxBackTacklePower()
            << "," << sp.playerSpeedMaxMin()
            << "," << sp.defaultExtraStamina()
            << "," << sp.synchSeeOffset()
            << "," << sp.maxMonitors()
            // 12.1.3
            << "," << sp.extraHalfTime()
            // 13.0.0
            << "," << sp.staminaCapacity()
            << "," << sp.maxDashAngle()
            << "," << sp.minDashAngle()
            << "," << sp.dashAngleStep()
            << "," << sp.sideDashRate()
            << "," << sp.backDashRate()
            << "," << sp.maxDashPower()
            << "," << sp.minDashPower()
            // 14.0.0
            << "," << sp.tackleRandFactor()
            << "," << sp.foulDetectProbability()
            << "," << sp.foulExponent()
            << "," << sp.foulCycles()
            << "," << sp.goldenGoal()
            // 15.0.0
            << "," << sp.redCardProbability()
            // 16.0.0
            << "," << sp.illegalDefenseDuration()
            << "," << sp.illegalDefenseNumber()
            << "," << sp.illegalDefenseDistX()
            << "," << sp.illegalDefenseWidth()
            << "," << "\"" << sp.fixedTeamNameLeft() << "\""
            << "," << "\"" << sp.fixedTeamNameRight() <<"\""
            ;
    M_os << std::endl;
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printPlayerParam() const
{
    const rcsc::PlayerParam& pp = rcsc::PlayerParam::i();
    //
    // Header
    //
    M_os    << "#" // Row index
            << ",player_types"
            << ",subs_max"
            << ",pt_max"
            << ",allow_mult_default_type"
            << ",player_speed_max_delta_min"
            << ",player_speed_max_delta_max"
            << ",stamina_inc_max_delta_factor"
            << ",player_decay_delta_min"
            << ",player_decay_delta_max"
            << ",inertia_moment_delta_factor"
            << ",dash_power_rate_delta_min"
            << ",dash_power_rate_delta_max"
            << ",player_size_delta_factor"
            << ",kickable_margin_delta_min"
            << ",kickable_margin_delta_max"
            << ",kick_rand_delta_factor"
            << ",extra_stamina_delta_min"
            << ",extra_stamina_delta_max"
            << ",effort_max_delta_factor"
            << ",effort_min_delta_factor"
            << ",random_seed"
            << ",new_dash_power_rate_delta_min"
            << ",new_dash_power_rate_delta_max"
            << ",new_stamina_inc_max_delta_factor"
            << ",kick_power_rate_delta_min"
            << ",kick_power_rate_delta_max"
            << ",foul_detect_probability_delta_factor"
            << ",catchable_area_l_stretch_min"
            << ",catchable_area_l_stretch_max"
            ;
    M_os << std::endl;
    //
    // Values
    //
    M_os    << "1"
            << "," << pp.playerTypes()
            << "," << pp.subsMax()
            << "," << pp.ptMax()
            << "," << pp.allowMultDefaultType()
            << "," << pp.playerSpeedMaxDeltaMin()
            << "," << pp.playerSpeedMaxDeltaMax()
            << "," << pp.staminaIncMaxDeltaFactor()
            << "," << pp.playerDecayDeltaMin()
            << "," << pp.playerDecayDeltaMax()
            << "," << pp.inertiaMomentDeltaFactor()
            << "," << pp.dashPowerRateDeltaMin()
            << "," << pp.dashPowerRateDeltaMax()
            << "," << pp.playerSizeDeltaFactor()
            << "," << pp.kickableMarginDeltaMin()
            << "," << pp.kickableMarginDeltaMax()
            << "," << pp.kickRandDeltaFactor()
            << "," << pp.extraStaminaDeltaMin()
            << "," << pp.extraStaminaDeltaMax()
            << "," << pp.effortMaxDeltaFactor()
            << "," << pp.effortMinDeltaFactor()
            << "," << pp.randomSeed()
            << "," << pp.newDashPowerRateDeltaMin()
            << "," << pp.newDashPowerRateDeltaMax()
            << "," << pp.newStaminaIncMaxDeltaFactor()
            << "," << pp.kickPowerRateDeltaMin()
            << "," << pp.kickPowerRateDeltaMax()
            << "," << pp.foulDetectProbabilityDeltaFactor()
            << "," << pp.catchAreaLengthStretchMin()
            << "," << pp.catchAreaLengthStretchMax()
            ;
    M_os << std::endl;
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printPlayerTypesHeader() const
{
    M_os    << "#"  // Row number
            << ",id"
            << ",player_speed_max"
            << ",stamina_inc_max"
            << ",player_decay"
            << ",inertia_moment"
            << ",dash_power_rate"
            << ",player_size"
            << ",kickable_margin"
            << ",kick_rand"
            << ",extra_stamina"
            << ",effort_max"
            << ",effort_min"
            << ",kick_power_rate"
            << ",foul_detect_probability"
            << ",catchable_area_l_stretch"
            ;
    M_os << std::endl;
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printPlayerType(const rcsc::PlayerType& type) const
{
    M_os    << playerTypesCount
            << "," << type.id()
            << "," << type.playerSpeedMax()
            << "," << type.staminaIncMax()
            << "," << type.playerDecay()
            << "," << type.inertiaMoment()
            << "," << type.dashPowerRate()
            << "," << type.playerSize()
            << "," << type.kickableMargin()
            << "," << type.kickRand()
            << "," << type.extraStamina()
            << "," << type.effortMax()
            << "," << type.effortMin()
            << "," << type.kickPowerRate()
            << "," << type.foulDetectProbability()
            << "," << type.catchAreaLengthStretch()
            ;
    M_os << std::endl;
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printShowHeader() const
{
    M_os << "#"
         << ", cycle, stopped"
         << ", playmode"
         << ", l_name, l_score, l_pen_score"
         << ", r_name, r_score, r_pen_score"
         << ", b_x, b_y, b_vx, b_vy";

    char side = 'l';
    for ( int s = 0; s < 2; ++s )
    {
        for ( int i = 1; i <= rcsc::MAX_PLAYER; ++i )
        {
            M_os << ", " << side << i << "_t"
                 << ", " << side << i << "_kick_tried"
                 << ", " << side << i << "_kick_failed"
                 << ", " << side << i << "_goalie"
                 << ", " << side << i << "_catch_tried"
                 << ", " << side << i << "_catch_failed"
                 << ", " << side << i << "_discarded"
                 << ", " << side << i << "_collided_with_ball"
                 << ", " << side << i << "_collided_with_player"
                 << ", " << side << i << "_tackle_tried"
                 << ", " << side << i << "_tackle_failed"
                 << ", " << side << i << "_backpassed"
                 << ", " << side << i << "_freekicked_wrong"
                 << ", " << side << i << "_collided_with_post"
                 << ", " << side << i << "_foul_frozen"
                 << ", " << side << i << "_yellow_card"
                 << ", " << side << i << "_red_card"
                 << ", " << side << i << "_defended_illegaly"
                 << ", " << side << i << "_x"
                 << ", " << side << i << "_y"
                 << ", " << side << i << "_vx"
                 << ", " << side << i << "_vy"
                 << ", " << side << i << "_body"
                 << ", " << side << i << "_neck"
                 << ", " << side << i << "_arm_point_x"
                 << ", " << side << i << "_arm_point_y"
                 << ", " << side << i << "_view_q"
                 << ", " << side << i << "_view_w"
                 << ", " << side << i << "_stamina"
                 << ", " << side << i << "_effort"
                 << ", " << side << i << "_stamina_rec"
                 << ", " << side << i << "_stamina_cap"
                 << ", " << side << i << "_focus_side"
                 << ", " << side << i << "_focus_unum"
                 << ", " << side << i << "_kick_count"
                 << ", " << side << i << "_dash_count"
                 << ", " << side << i << "_turn_count"
                 << ", " << side << i << "_catch_count"
                 << ", " << side << i << "_move_count"
                 << ", " << side << i << "_turnneck_count"
                 << ", " << side << i << "_changeview_count"
                 << ", " << side << i << "_say_count"
                 << ", " << side << i << "_tackle_count"
                 << ", " << side << i << "_arm_count"
                 << ", " << side << i << "_focus_count";
        }
        side = 'r';
    }

    M_os << '\n';
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printShowData( const rcsc::rcg::ShowInfoT & show ) const
{
    printShowCount();
    printTime();
    printPlayMode();
    printTeams();
    printBall( show.ball_ );
    printPlayers( show );

    M_os << '\n';
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printShowCount() const
{
    M_os << M_show_count;
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printTime() const
{
    M_os << ',' << M_cycle << ',' << M_stopped;
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printPlayMode() const
{
    M_os << ',' << getPlayModeString( M_playmode );
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printTeams() const
{
    for ( const auto & t : M_teams )
    {
        M_os << ',' << t.name_
             << ',' << t.score_
             << ',' << t.pen_score_;
    }

    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printBall( const rcsc::rcg::BallT & ball ) const
{
    M_os << ',' << ball.x_
         << ',' << ball.y_
         << ',' << ball.vx_
         << ',' << ball.vy_;
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printPlayers( const rcsc::rcg::ShowInfoT & show ) const
{
    for ( const auto & p : show.player_ )
    {
        printPlayer( p );
    }

    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printPlayer( const rcsc::rcg::PlayerT & player ) const
{
    if (!player.isAlive())
    {
        // Player is disconnected
        M_os << "," //<< player.type_
             << "," //<< ((player.state_ & rcsc::rcg::KICK) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::KICK_FAULT) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::GOALIE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::CATCH) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::CATCH_FAULT) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::DISCARD) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::BALL_COLLIDE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::PLAYER_COLLIDE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::TACKLE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::TACKLE_FAULT) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::BACK_PASS) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::FREE_KICK_FAULT) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::POST_COLLIDE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::FOUL_CHARGED) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::YELLOW_CARD) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::RED_CARD) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::ILLEGAL_DEFENSE) != 0)
             << ',' //<< player.x_
             << ',' //<< player.y_
             << ',' //<< player.vx_
             << ',' //<< player.vy_
             << ',' //<< player.body_
             << ',' //<< player.neck_
             << "," //<< player.point_x_ 
             << "," //<< player.point_y_
             << "," //<< player.view_quality_
             << "," //<< player.view_width_
             << "," //<< player.stamina_
             << "," //<< player.effort_
             << "," //<< player.recovery_
             << "," //<< player.stamina_capacity_
             << "," //<< player.focus_side_
             << "," //<< player.focus_unum_
             << "," //<< player.kick_count_
             << "," //<< player.dash_count_
             << "," //<< player.turn_count_
             << "," //<< player.catch_count_
             << "," //<< player.move_count_
             << "," //<< player.turn_neck_count_
             << "," //<< player.change_view_count_
             << "," //<< player.say_count_
             << "," //<< player.tackle_count_
             << "," //<< player.pointto_count_
             << "," //<< player.attentionto_count_
            ;
    } 
    else if (player.state_ & rcsc::rcg::DISCARD)
    {
        // Player was either discarded by monitor or received a red card
        M_os << "," //<< player.type_
             << "," //<< ((player.state_ & rcsc::rcg::KICK) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::KICK_FAULT) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::GOALIE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::CATCH) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::CATCH_FAULT) != 0)
             << "," << ((player.state_ & rcsc::rcg::DISCARD) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::BALL_COLLIDE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::PLAYER_COLLIDE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::TACKLE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::TACKLE_FAULT) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::BACK_PASS) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::FREE_KICK_FAULT) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::POST_COLLIDE) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::FOUL_CHARGED) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::YELLOW_CARD) != 0)
             << "," << ((player.state_ & rcsc::rcg::RED_CARD) != 0)
             << "," //<< ((player.state_ & rcsc::rcg::ILLEGAL_DEFENSE) != 0)
             << ',' //<< player.x_
             << ',' //<< player.y_
             << ',' //<< player.vx_
             << ',' //<< player.vy_
             << ',' //<< player.body_
             << ',' //<< player.neck_
             << "," //<< player.point_x_ 
             << "," //<< player.point_y_
             << "," //<< player.view_quality_
             << "," //<< player.view_width_
             << "," //<< player.stamina_
             << "," //<< player.effort_
             << "," //<< player.recovery_
             << "," //<< player.stamina_capacity_
             << "," //<< player.focus_side_
             << "," //<< player.focus_unum_
             << "," //<< player.kick_count_
             << "," //<< player.dash_count_
             << "," //<< player.turn_count_
             << "," //<< player.catch_count_
             << "," //<< player.move_count_
             << "," //<< player.turn_neck_count_
             << "," //<< player.change_view_count_
             << "," //<< player.say_count_
             << "," //<< player.tackle_count_
             << "," //<< player.pointto_count_
             << "," //<< player.attentionto_count_
            ;
    }
    else
    {
        M_os << "," << player.type_
             << "," << ((player.state_ & rcsc::rcg::KICK) != 0)
             << "," << ((player.state_ & rcsc::rcg::KICK_FAULT) != 0)
             << "," << ((player.state_ & rcsc::rcg::GOALIE) != 0)
             << "," << ((player.state_ & rcsc::rcg::CATCH) != 0)
             << "," << ((player.state_ & rcsc::rcg::CATCH_FAULT) != 0)
             << "," << ((player.state_ & rcsc::rcg::DISCARD) != 0)
             << "," << ((player.state_ & rcsc::rcg::BALL_COLLIDE) != 0)
             << "," << ((player.state_ & rcsc::rcg::PLAYER_COLLIDE) != 0)
             << "," << ((player.state_ & rcsc::rcg::TACKLE) != 0)
             << "," << ((player.state_ & rcsc::rcg::TACKLE_FAULT) != 0)
             << "," << ((player.state_ & rcsc::rcg::BACK_PASS) != 0)
             << "," << ((player.state_ & rcsc::rcg::FREE_KICK_FAULT) != 0)
             << "," << ((player.state_ & rcsc::rcg::POST_COLLIDE) != 0)
             << "," << ((player.state_ & rcsc::rcg::FOUL_CHARGED) != 0)
             << "," << ((player.state_ & rcsc::rcg::YELLOW_CARD) != 0)
             << "," << ((player.state_ & rcsc::rcg::RED_CARD) != 0)
             << "," << ((player.state_ & rcsc::rcg::ILLEGAL_DEFENSE) != 0)
             << ',' << player.x_
             << ',' << player.y_
             << ',' << player.vx_
             << ',' << player.vy_
             << ',' << player.body_
             << ',' << player.neck_
            ;
        if (player.isPointing())
        {
            M_os << "," << player.point_x_ 
                 << "," << player.point_y_
                ;
        }
        else
        {
            M_os << "," //<< player.point_x_ 
                 << "," //<< player.point_y_
                ;
        }
        M_os << "," << player.view_quality_
             << "," << player.view_width_
             << "," << player.stamina_
             << "," << player.effort_
             << "," << player.recovery_
             << "," << player.stamina_capacity_
            ;
        if (player.isFocusing())
        {
            M_os << "," << player.focus_side_
                 << "," << player.focus_unum_
                ;
        }
        else
        {
            M_os << "," //<< player.focus_side_
                 << "," //<< player.focus_unum_
                ;
        }
        M_os << "," << player.kick_count_
             << "," << player.dash_count_
             << "," << player.turn_count_
             << "," << player.catch_count_
             << "," << player.move_count_
             << "," << player.turn_neck_count_
             << "," << player.change_view_count_
             << "," << player.say_count_
             << "," << player.tackle_count_
             << "," << player.pointto_count_
             << "," << player.attentionto_count_
            ;
    }
    return M_os;
}

////////////////////////////////////////////////////////////////////////

class MultiSinkCSVPrinter 
    : public rcsc::rcg::Handler {
public:
    MultiSinkCSVPrinter();
    MultiSinkCSVPrinter(const MultiSinkCSVPrinter&) = delete;
    MultiSinkCSVPrinter(MultiSinkCSVPrinter&&) = delete;

    virtual
    ~MultiSinkCSVPrinter();

    virtual
    bool handleShow( const rcsc::rcg::ShowInfoT & show ) noexcept override {
        return matchPrinter ? matchPrinter->handleShow(show) : true;
    }

    virtual
    bool handleMsg( const int time,
                    const int board,
                    const std::string & msg ) noexcept override {
        return matchPrinter ? matchPrinter->handleMsg(time, board, msg) : true;
    }

    virtual
    bool handleDraw( const int time,
                     const rcsc::rcg::drawinfo_t & draw ) noexcept override {
        return matchPrinter ? matchPrinter->handleDraw(time, draw) : true;
    }

    virtual
    bool handlePlayMode( const int time,
                         const rcsc::PlayMode pm ) noexcept override {
        return matchPrinter ? matchPrinter->handlePlayMode(time, pm) : true;
    }

    virtual
    bool handleTeam( const int time,
                     const rcsc::rcg::TeamT & team_l,
                     const rcsc::rcg::TeamT & team_r ) noexcept override {
        return matchPrinter ? matchPrinter->handleTeam(time, team_l, team_r) : true;
    }

    virtual
    bool handleServerParam( const std::string & msg ) noexcept override {
        return serverParamsPrinter ? serverParamsPrinter->handleServerParam(msg) : true;
    }

    virtual
    bool handlePlayerParam( const std::string & msg ) noexcept override {
        return playerParamsPrinter ? playerParamsPrinter->handlePlayerParam(msg) : true;
    }

    virtual
    bool handlePlayerType( const std::string & msg ) noexcept override {
        return playerTypesPrinter ? playerTypesPrinter->handlePlayerType(msg) : true;
    }

    virtual
    bool handleEOF() noexcept override {
        return matchPrinter ? matchPrinter->handleEOF() : true;
    }

    /*!
      \brief Enables printing the Match CSV Table and sets its output destination.
      \param sink The output destination. If null, the printer prints to std::cout.
    */
    void enableMatchPrinter(std::unique_ptr<std::ostream>&& sink=nullptr) noexcept;

    /*!
      \brief Enables printing the ServerParams CSV Table and sets its output destination.
      \param sink The output destination. If null, the printer prints to std::cout.
    */
    void enableServerParamsPrinter(std::unique_ptr<std::ostream>&& sink=nullptr) noexcept;

    /*!
      \brief Enables printing the PlayerParams CSV Table and sets its output destination.
      \param sink The output destination. If null, the printer prints to std::cout.
    */
    void enablePlayerParamsPrinter(std::unique_ptr<std::ostream>&& sink=nullptr) noexcept;

    /*!
      \brief Enables printing the PlayerTypes CSV Table and sets its output destination.
      \param sink The output destination. If null, the printer prints to std::cout.
    */
    void enablePlayerTypesPrinter(std::unique_ptr<std::ostream>&& sink=nullptr) noexcept;

private:

    std::unique_ptr<CSVPrinter> matchPrinter; //<! Prints the CSV table with data from the course of the match
    std::unique_ptr<std::ostream> matchPrinterSink; //<! The output sink for the match table
    std::unique_ptr<CSVPrinter> serverParamsPrinter; //<! Prints the CSV table with rcssserver used server parameters
    std::unique_ptr<std::ostream> serverParamsSink; //<! The output sink for the server params table
    std::unique_ptr<CSVPrinter> playerParamsPrinter; //<! Prints the CSV table with rcssserver used player parameters
    std::unique_ptr<std::ostream> playerParamsSink; //<! The output sink for the player params table
    std::unique_ptr<CSVPrinter> playerTypesPrinter; //<! Prints the CSV table with the player types available in the match
    std::unique_ptr<std::ostream> playerTypesSink; //<! The output sink for the player types table
};

MultiSinkCSVPrinter::MultiSinkCSVPrinter()
:   matchPrinter(nullptr)
,   matchPrinterSink(nullptr)
,   serverParamsPrinter(nullptr)
,   serverParamsSink(nullptr)
,   playerParamsPrinter(nullptr)
,   playerParamsSink(nullptr)
,   playerTypesPrinter(nullptr)
,   playerTypesSink(nullptr)
{}

MultiSinkCSVPrinter::~MultiSinkCSVPrinter() {
    auto matchFileSink  = dynamic_cast<std::ofstream*>(matchPrinterSink.get());
    if (matchFileSink) {
        matchFileSink->close();
    }
    auto serverParamsFileSink = dynamic_cast<std::ofstream*>(serverParamsSink.get());
    if (serverParamsFileSink) {
        serverParamsFileSink->close();
    }
    auto playerParamsFileSink = dynamic_cast<std::ofstream*>(playerParamsSink.get());
    if (playerParamsFileSink) {
        playerParamsFileSink->close();
    }
    auto playerTypesFileSink = dynamic_cast<std::ofstream*>(playerTypesSink.get());
    if (playerTypesFileSink) {
        playerTypesFileSink->close();
    }
}

void 
MultiSinkCSVPrinter::enableMatchPrinter(std::unique_ptr<std::ostream>&& sink) noexcept {
    matchPrinterSink = std::forward<std::unique_ptr<std::ostream>>(sink);
    matchPrinter.reset( new CSVPrinter( matchPrinterSink ? *matchPrinterSink : std::cout) );
}

void 
MultiSinkCSVPrinter::enableServerParamsPrinter(std::unique_ptr<std::ostream>&& sink) noexcept {
    serverParamsSink = std::forward<std::unique_ptr<std::ostream>>(sink);
    serverParamsPrinter.reset( new CSVPrinter( serverParamsSink ? *serverParamsSink : std::cout) );
}

void 
MultiSinkCSVPrinter::enablePlayerParamsPrinter(std::unique_ptr<std::ostream>&& sink) noexcept {
    playerParamsSink = std::forward<std::unique_ptr<std::ostream>>(sink);
    playerParamsPrinter.reset( new CSVPrinter( playerParamsSink ? *playerParamsSink : std::cout) );
}

void 
MultiSinkCSVPrinter::enablePlayerTypesPrinter(std::unique_ptr<std::ostream>&& sink) noexcept {
    playerTypesSink = std::forward<std::unique_ptr<std::ostream>>(sink);
    playerTypesPrinter.reset( new CSVPrinter( playerTypesSink ? *playerTypesSink : std::cout) );
}

////////////////////////////////////////////////////////////////////////

class RCG2CSVOptions {
public:

    RCG2CSVOptions() = default;
    RCG2CSVOptions(const RCG2CSVOptions&) = default;
    RCG2CSVOptions(RCG2CSVOptions&&) = default;

    bool matchTableEnabled() const noexcept {
        return matchTableSwitch;
    }
    const std::string& getMatchTableOutputPath() const noexcept {
        return matchTableOutputPath;
    }
    bool serverParamsTableEnabled() const noexcept {
        return serverParamsTableSwitch;
    }
    const std::string& getServerParamsTableOutputPath() const noexcept {
        return serverParamsTableOutputPath;
    }
    bool playerParamsTableEnabled() const noexcept {
        return playerParamsTableSwitch;
    }
    const std::string& getPlayerParamsTableOutputPath() const noexcept {
        return playerParamsTableOutputPath;
    }
    bool playerTypesTableEnabled() const noexcept {
        return playerTypesTableSwitch;
    }
    const std::string& getPlayerTypesTableOutputPath() const noexcept {
        return playerTypesTableOutputPath;
    }
    const std::string& getRCGSourcePath() const noexcept {
        return rcgSourcePath;
    }

    //<! Below is just a commodity so we don't add lots of setters.
    friend void fillFromCmdLine(int, const char* const*, RCG2CSVOptions&);
private:
    bool matchTableSwitch = false;
    std::string matchTableOutputPath;
    bool serverParamsTableSwitch = false;
    std::string serverParamsTableOutputPath;
    bool playerParamsTableSwitch = false;
    std::string playerParamsTableOutputPath;
    bool playerTypesTableSwitch = false;
    std::string playerTypesTableOutputPath;
    std::string rcgSourcePath;
};

////////////////////////////////////////////////////////////////////////

void usage()
{
    std::cerr << "Usage: rcg2csv [options] <RCGFile>[.gz]" << std::endl;
}

void fillFromCmdLine(int argc, const char* const* argv, RCG2CSVOptions& options)
{
    // Define options for command line arguments and register capture variables.
    rcsc::ParamMap rcg2csvParamMap;
    bool help = false;
    rcg2csvParamMap.add()
        ("help", "h", rcsc::BoolSwitch(&help), "Display help message and exit.")
        ("match", "m", rcsc::BoolSwitch(&options.matchTableSwitch), "Print CSV table with match development data.")
        ("match-out", "mo", &options.matchTableOutputPath, "Output path for the Match table. Leave empty to use the standard output.")
        ("serverparams", "sp", rcsc::BoolSwitch(&options.serverParamsTableSwitch), "Print CSV table with rcssserver server params used in the match.")
        ("serverparams-out", "spo", &options.serverParamsTableOutputPath, "Output path for the ServerParams table. Leave empty to use the standard output.")
        ("playerparams", "pp", rcsc::BoolSwitch(&options.playerParamsTableSwitch), "Print CSV table with rcssserver player params used in the match.")
        ("playerparams-out", "ppo", &options.playerParamsTableOutputPath, "Output path for the PlayerParams table. Leave empty to use the standard output.")
        ("playertypes", "pt", rcsc::BoolSwitch(&options.playerTypesTableSwitch), "Print CSV table with player types available in the match.")
        ("playertypes-out", "pto", &options.playerTypesTableOutputPath, "Output path for the PlayerTypes table. Leave empty to use the standard output.")
        ;
    // The capture variables default values are the ones stored before parsing.
    // If we don't generate the help message before parsing, we lose them and will display a wrong help message.
    std::stringstream helpMessage;
    rcg2csvParamMap.printHelp(helpMessage);

    // Parse command line arguments and fill parameter map
    rcsc::CmdLineParser cmdLineParser(argc, argv);
    cmdLineParser.parse(rcg2csvParamMap);
    if (help) {
        usage();
        std::cerr << helpMessage.str();
        exit(0);
    } else if (cmdLineParser.failed()) {
        std::cerr << "ERROR: Failed to parse Command Line Options." << std::endl;
        usage();
        std::cerr << helpMessage.str();
        exit(1);
    }

    // Collect RCG source path
    if (cmdLineParser.positionalOptions().size() != 1) {
        std::cerr << "ERROR: Expected 1 positional argument." << std::endl;
        usage();
        std::cerr << helpMessage.str();
        exit(1);
    }
    options.rcgSourcePath = cmdLineParser.positionalOptions().front();

    // Exit with error if multiple tables will print at standard output
    std::vector<bool> enabled{
        options.matchTableSwitch, 
        options.serverParamsTableSwitch,
        options.playerParamsTableSwitch,
        options.playerTypesTableSwitch
    };
    std::vector<std::string> sinkPathStrs{
        options.matchTableOutputPath, 
        options.serverParamsTableOutputPath,
        options.playerParamsTableOutputPath,
        options.playerTypesTableOutputPath
    };
    assert(enabled.size() == sinkPathStrs.size());
    int count = 0;
    for (size_t i=0; i<enabled.size(); i++) {
        if (enabled[i] && sinkPathStrs[i].empty())
            count++;
    }
    if (count > 1) {
        std::cerr << "ERROR: Cannot print multiple tables into standard output." << std::endl;
        usage();
        std::cerr << helpMessage.str();
        exit(1);
    }
}


int
main( int argc, char** argv )
{
    RCG2CSVOptions options;
    fillFromCmdLine(argc, argv, options);

    // Warn if no work to be done.
    if (!options.matchTableEnabled() 
    && !options.serverParamsTableEnabled() 
    && !options.playerParamsTableEnabled()
    && !options.playerTypesTableEnabled())
    {
        std::cerr << "WARNING: No work to be done." << std::endl;
    }


    rcsc::gzifstream fin( options.getRCGSourcePath().c_str() );

    if ( ! fin.is_open() )
    {
        std::cerr << "ERROR: Failed to open file : " << options.getRCGSourcePath() << std::endl;
        return 1;
    }

    rcsc::rcg::Parser::Ptr parser = rcsc::rcg::Parser::create( fin );

    if ( ! parser )
    {
        std::cerr << "ERROR: Failed to create rcg parser." << std::endl;
        return 1;
    }


    MultiSinkCSVPrinter printer;
    // Initialize MultiSink Printer
    // Match
    if (options.matchTableEnabled()) {
        if (options.getMatchTableOutputPath().empty()) {
            printer.enableMatchPrinter();
        } else {
            std::unique_ptr<std::ostream> matchTableSink(new std::ofstream(options.getMatchTableOutputPath()));
            if (matchTableSink->fail()) {
                std::cerr << "ERROR: Could not open match table output file \"" << options.getMatchTableOutputPath() << "\"" << std::endl;
                return 1;
            }
            printer.enableMatchPrinter(std::move(matchTableSink));
        }
    }
    // Server Parameters
    if (options.serverParamsTableEnabled()) {
        if (options.getServerParamsTableOutputPath().empty()) {
            printer.enableServerParamsPrinter();
        } else {
            std::unique_ptr<std::ostream> serverParamsTableSink(new std::ofstream(options.getServerParamsTableOutputPath()));
            if (serverParamsTableSink->fail()) {
                std::cerr << "ERROR: Could not open server params table output file \"" << options.getServerParamsTableOutputPath() << "\"" << std::endl;
                return 1;
            }
            printer.enableServerParamsPrinter(std::move(serverParamsTableSink));
        }
    }
    // Player Parameters
    if (options.playerParamsTableEnabled()) {
        if (options.getPlayerParamsTableOutputPath().empty()) {
            printer.enablePlayerParamsPrinter();
        } else {
            std::unique_ptr<std::ostream> playerParamsTableSink(new std::ofstream(options.getPlayerParamsTableOutputPath()));
            if (playerParamsTableSink->fail()) {
                std::cerr << "ERROR: Could not open player params table output file \"" << options.getPlayerParamsTableOutputPath() << "\"" << std::endl;
                return 1;
            }
            printer.enablePlayerParamsPrinter(std::move(playerParamsTableSink));
        }
    }
    // Player Types
    if (options.playerTypesTableEnabled()) {
        if (options.getPlayerTypesTableOutputPath().empty()) {
            printer.enablePlayerTypesPrinter();
        } else {
            std::unique_ptr<std::ostream> playerTypesTableSink(new std::ofstream(options.getPlayerTypesTableOutputPath()));
            if (playerTypesTableSink->fail()) {
                std::cerr << "ERROR: Could not open player params table output file \"" << options.getPlayerTypesTableOutputPath() << "\"" << std::endl;
                return 1;
            }
            printer.enablePlayerTypesPrinter(std::move(playerTypesTableSink));
        }
    }

    parser->parse( fin, printer );

    return 0;
}
