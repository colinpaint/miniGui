// cSongLoader.h - video, audio - loader,player
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <functional>
#include <vector>

class cLoadSource;
class cSong;
class iVideoPool;
//}}}

class iSongLoad {
public:
  virtual ~iSongLoad() {}

  virtual cSong* getSong() const = 0;
  virtual iVideoPool* getVideoPool() const = 0;
  virtual std::string getInfoString() = 0;
  virtual float getFracs (float& audioFrac, float& videoFrac) = 0;

  // actions
  virtual bool togglePlaying() = 0;
  virtual bool skipBegin() = 0;
  virtual bool skipEnd() = 0;
  virtual bool skipBack (bool shift, bool control) = 0;
  virtual bool skipForward (bool shift, bool control) = 0;
  virtual void exit() = 0;
  };

class cSongLoader : public iSongLoad {
public:
  cSongLoader();
  virtual ~cSongLoader();

  // iLoad gets
  virtual cSong* getSong() const override;
  virtual iVideoPool* getVideoPool() const override;
  virtual std::string getInfoString() override;
  virtual float getFracs (float& audioFrac, float& videoFrac) override;

  // iLoad actions
  virtual bool togglePlaying() override;
  virtual bool skipBegin() override;
  virtual bool skipEnd() override;
  virtual bool skipBack (bool shift, bool control) override;
  virtual bool skipForward (bool shift, bool control) override;
  virtual void exit() override;

  // load
  void load (const std::vector<std::string>& params, std::function <void(int64_t)>& playCallback);
  void launchLoad (const std::vector<std::string>& params, std::function <void(int64_t)>& playCallback);

private:
  cLoadSource* mLoadSource = nullptr;
  cLoadSource* mLoadIdle = nullptr;
  std::vector <cLoadSource*> mLoadSources;
  };
