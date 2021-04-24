// -*-c++-*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
    std::ostream & printPlayerTypes() const;


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
      M_playmode( rcsc::PM_Null )
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
    if ( ! rcsc::ServerParam::instance().parse( msg.c_str(), 8 ) )
    {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handlePlayerParam( const std::string & msg )
{
    if ( ! rcsc::PlayerParam::instance().parse( msg.c_str(), 8 ) )
    {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
CSVPrinter::handlePlayerType( const std::string & )
{
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
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printPlayerParam() const
{
    return M_os;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
CSVPrinter::printPlayerTypes() const
{
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
        return matchPrinter ? matchPrinter->handleServerParam(msg) : true;
    }

    virtual
    bool handlePlayerParam( const std::string & msg ) noexcept override {
        return matchPrinter ? matchPrinter->handlePlayerParam(msg) : true;
    }

    virtual
    bool handlePlayerType( const std::string & msg ) noexcept override {
        return matchPrinter ? matchPrinter->handlePlayerType(msg) : true;
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

private:

    std::unique_ptr<CSVPrinter> matchPrinter; //<! Prints the CSV table with data from the course of the match
    std::unique_ptr<std::ostream> matchPrinterSink; //<! The output sink for the match table
};

MultiSinkCSVPrinter::MultiSinkCSVPrinter()
:   matchPrinter(nullptr)
,   matchPrinterSink(nullptr)
{}

void 
MultiSinkCSVPrinter::enableMatchPrinter(std::unique_ptr<std::ostream>&& sink) noexcept {
    matchPrinterSink = std::forward<std::unique_ptr<std::ostream>>(sink);
    matchPrinter.reset( new CSVPrinter( matchPrinterSink ? *matchPrinterSink : std::cout) );
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
    const std::string& getRCGSourcePath() const noexcept {
        return rcgSourcePath;
    }

    //<! Below is just a commodity so we don't add lots of setters.
    friend void fillFromCmdLine(int, const char* const*, RCG2CSVOptions&);
private:
    bool matchTableSwitch = false;
    std::string matchTableOutputPath;
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
}


int
main( int argc, char** argv )
{
    RCG2CSVOptions options;
    fillFromCmdLine(argc, argv, options);

    // Warn if no work to be done.
    if (!options.matchTableEnabled()) {
        std::cerr << "WARNING: No work to be done." << std::endl;
    }

    rcsc::gzifstream fin( options.getRCGSourcePath().c_str() );

    if ( ! fin.is_open() )
    {
        std::cerr << "Failed to open file : " << options.getRCGSourcePath() << std::endl;
        return 1;
    }

    rcsc::rcg::Parser::Ptr parser = rcsc::rcg::Parser::create( fin );

    if ( ! parser )
    {
        std::cerr << "Failed to create rcg parser." << std::endl;
        return 1;
    }


    MultiSinkCSVPrinter printer;
    // Initialize MultiSink Printer
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

    parser->parse( fin, printer );

    return 0;
}
