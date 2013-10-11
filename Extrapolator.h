//  Entity Position Interpolation Code library.
//  Copyright 2006 Jon Watte, All Rights Reserved.
//  You may use this code, free of charge, for your own 
//  projects, under certain conditions ("MIT License")
//  explained in the file LICENSE included with this 
//  source code. If you do not have this file, see the 
//  web site http://www.mindcontrol.org/~hplus/epic/
//  for license conditions.
#if !defined(EPIC_Extrapolator_h)
#define EPIC_Extrapolator_h

//! Extrapolator maintains state about updates for remote entities, and 
//! will generate smooth guesses about where those entities will be 
//! based on previously received data.
//! 
//! You create one Extrapolator per quantity you want to interpolate. 
//! Arguments include the type to interpolate (works best with float 
//! or double), and how many values/axes (typically 3 for positions).
//!
//! You then feed it updates about where your entity was ("positions") 
//! at some point in time. Optionally, you can also give the Extrapolator
//! information about the velocity at that point in time.
//! After that, you can ask the Extrapolator for position values for 
//! some given real time. Optionally, you can also ask for the velocity 
//! at the same time.
//!
//! The Extrapolator will assume that an entity stops in place if it 
//! hasn't received updates for some time about the entity. It will also 
//! keep a running estimate of the latency and frequency of updates.
//!
//! Extrapolator requires a globally synchronized clock. It does not do 
//! any management to deal with clock skew (that is the subject of a 
//! separate class).
template<int Count = 3, typename Type = float>
class Extrapolator {
  public:
    //! Create the Extrapolator, initializing all fields and guesses to 0.
    Extrapolator();
    //! Bye, bye, Extrapolator!
    ~Extrapolator();
    //! When you receive data about a remote entity, call AddSample() to 
    //! tell the Extrapolator about it. The Extrapolator will throw away a 
    //! sample with a time that's before any other time it's already gotten, 
    //! and return false; else it will use the sample to improve its 
    //! interpolation guess, and return true.
    //! \param packetTime The globally synchronized time at which the 
    //! packet was sent (and thus the data valid).
    //! \param curTime The globally synchronized time at which you put 
    //! the data into the Extrapolator (i e, "now").
    //! \param pos The position sample valid for packetTime.
    //! \return true if packetTime is strictly greater than the previous 
    //! packetTime, else false.
    bool AddSample(double packetTime, double curTime, Type const pos[Count]);
    //! When you receive data about a remote entity, call AddSample() to 
    //! tell the Extrapolator about it. The Extrapolator will throw away a 
    //! sample with a time that's before any other time it's already gotten, 
    //! and return false; else it will use the sample to improve its 
    //! interpolation guess, and return true.
    //!
    //! If you get velocity information with your position updates, you can 
    //! make the guess that Extrapolator makes better, by passing that 
    //! information along with your position sample.
    //! \param packetTime The globally synchronized time at which the 
    //! packet was sent (and thus the data valid).
    //! \param curTime The globally synchronized time at which you put 
    //! the data into the Extrapolator (i e, "now").
    //! \param pos The position sample valid for packetTime.
    //! \return true if packetTime is strictly greater than the previous 
    //! packetTime, else false.
    //! \param vel The velocity of the entity at the time of packetTime.
    //! Used to improve the guess about entity position.
    bool AddSample(double packetTime, double curTime, Type const pos[Count], Type const vel[Count]);
    //! Re-set the Extrapolator's idea of time, velocity and position.
    //! \param packetTime The packet time to reset to.
    //! \param curTime The current time to reset to.
    //! \param pos The position to reset to.
    //! \note The velocity will be re-set to 0.
    void Reset(double packetTime, double curTime, Type const pos[Count]);
    //! Re-set the Extrapolator's idea of time, velocity and position.
    //! \param packetTime The packet time to reset to.
    //! \param curTime The current time to reset to.
    //! \param pos The position to reset to.
    //! \param vel The velocity to reset to.
    void Reset(double packetTime, double curTime, Type const pos[Count], Type const vel[Count]);
    //! Return an estimate of the interpolated position at a given global 
    //! time (which typically will be greater than the curTime passed into 
    //! AddSample()).
    //! \param forTime The time at which to interpolate the entity. It should 
    //! be greater than the last packetTime, and less than the last curTime 
    //! plus some allowable slop (determined by EstimateFreqency()).
    //! \param oPos The interpolated position for the given time.
    //! \return false if forTime is out of range (at which point the oPos 
    //! will still make sense, but movement will be clamped); true when forTime
    //! is within range.
    bool ReadPosition(double forTime, Type oPos[Count]) const;
    //! Return an estimate of the interpolated position at a given global 
    //! time (which typically will be greater than the curTime passed into 
    //! AddSample()).
    //! \param forTime The time at which to interpolate the entity. It should 
    //! be greater than the last packetTime, and less than the last curTime 
    //! plus some allowable slop (determined by EstimateFreqency()).
    //! \param oPos The interpolated position for the given time.
    //! \param oVel The interpolated velocity for the given time.
    //! \return false if forTime is out of range (at which point the oPos 
    //! will still make sense, but velocity will be zero); true when forTime
    //! is within range.
    bool ReadPosition(double forTime, Type oPos[Count], Type oVel[Count]) const;
    //! \return the current estimation of latency between the sender and this
    //! interpolator. This is updated after each AddSample(), and re-set 
    //! on Reset().
    double EstimateLatency() const;
    //! \return the current estimation of frequency of updates that the sender 
    //! will send. This is updated after each AddSample(), and re-set on Reset().
    double EstimateUpdateTime() const;
  private:
    Type snapPos_[Count];
    Type snapVel_[Count];
    Type aimPos_[Count];
    Type lastPacketPos_[Count];     //  only used when re-constituting velocity
    double snapTime_;               //  related to snapPos_
    double aimTime_;                //  related to aimPos_
    double lastPacketTime_;         //  related to lastPacketPos_
    double latency_;
    double updateTime_;
    bool Estimates(double packet, double cur);
};

#include "Extrapolator.cpp"

#endif
