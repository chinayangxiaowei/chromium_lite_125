// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_BIRCH_BIRCH_MODEL_H_
#define ASH_BIRCH_BIRCH_MODEL_H_

#include <map>
#include <optional>
#include <vector>

#include "ash/ash_export.h"
#include "ash/birch/birch_client.h"
#include "ash/birch/birch_item.h"
#include "ash/public/cpp/session/session_observer.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chromeos/ash/components/geolocation/simple_geolocation_provider.h"
#include "components/prefs/pref_change_registrar.h"

class PrefRegistrySimple;

namespace ash {

class BirchDataProvider;
class BirchItemRemover;

// Birch model, which is used to aggregate and store relevant information from
// different providers. Both data and prefs are associated with the primary user
// account.
class ASH_EXPORT BirchModel : public SessionObserver,
                              public SimpleGeolocationProvider::Observer {
 public:
  // BirchModel Observers are notified when the BirchClient has been set.
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override = default;

    virtual void OnBirchClientSet() = 0;
  };

  BirchModel();
  BirchModel(const BirchModel&) = delete;
  BirchModel& operator=(const BirchModel&) = delete;
  ~BirchModel() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Sends a request to the birch keyed service to fetch data into the model.
  // `is_post_login` determines fetch timeout depending on whether this request
  // is made post login.
  // `callback` will run once either all data is fresh or the request timeout
  // has expired.
  void RequestBirchDataFetch(bool is_post_login, base::OnceClosure callback);

  void SetCalendarItems(const std::vector<BirchCalendarItem>& calendar_items);
  void SetAttachmentItems(
      const std::vector<BirchAttachmentItem>& attachment_items);
  void SetFileSuggestItems(
      const std::vector<BirchFileItem>& file_suggest_items);
  void SetRecentTabItems(const std::vector<BirchTabItem>& recent_tab_items);
  void SetReleaseNotesItems(
      const std::vector<BirchReleaseNotesItem>& release_notes_items);
  void SetWeatherItems(const std::vector<BirchWeatherItem>& weather_items);

  // Sets the BirchClient and begins initializing the BirchItemRemover.
  void SetClientAndInit(BirchClient* client);

  BirchClient* birch_client() { return birch_client_; }

  const std::vector<BirchCalendarItem>& GetCalendarItemsForTest() const {
    return calendar_items_;
  }
  const std::vector<BirchAttachmentItem>& GetAttachmentItemsForTest() const {
    return attachment_items_;
  }
  const std::vector<BirchFileItem>& GetFileSuggestItemsForTest() const {
    return file_suggest_items_;
  }
  const std::vector<BirchTabItem>& GetTabsForTest() const {
    return recent_tab_items_;
  }
  const std::vector<BirchReleaseNotesItem>& GetReleaseNotesItemsForTest()
      const {
    return release_notes_items_;
  }
  const std::vector<BirchWeatherItem>& GetWeatherForTest() const {
    return weather_items_;
  }
  BirchItemRemover* GetItemRemoverForTest() { return item_remover_.get(); }

  // Returns all items, sorted by ranking. Includes unranked items.
  std::vector<std::unique_ptr<BirchItem>> GetAllItems();

  // Returns all items, sorted by ranking.
  std::vector<std::unique_ptr<BirchItem>> GetItemsForDisplay();

  // Returns whether all data in the model is currently fresh.
  bool IsDataFresh();

  // Add the BirchItem to the list of persistenly removed items.
  void RemoveItem(BirchItem* item);

  // SessionObserver:
  void OnActiveUserSessionChanged(const AccountId& account_id) override;

  // SimpleGeolocationProvider::Observer:
  void OnGeolocationPermissionChanged(bool enabled) override;

  void OverrideWeatherProviderForTest(
      std::unique_ptr<BirchDataProvider> weather_provider);
  void OverrideClockForTest(base::Clock* clock);

 private:
  friend class BirchModelTest;

  // Timer and callback for a pending data fetch request.
  // The callback will be run if the timer expires before all data is fetched.
  struct PendingRequest {
    PendingRequest();
    ~PendingRequest();

    base::OnceClosure callback;
    std::unique_ptr<base::OneShotTimer> timer;
  };

  // Called when a pending data fetch request timeout expires.
  void HandleRequestTimeout(size_t request_id);

  // Runs data fetch callbacks after a data fetch request when all data items
  // have been refreshed.
  void MaybeRespondToDataFetchRequest();

  // Get current time. The clock may be overridden for testing purposes.
  base::Time GetNow() const;

  // Clears all items.
  void ClearAllItems();

  // Marks all data types as not fresh.
  void MarkDataNotFresh();

  // Initializes the pref change registrars to observe for pref changes.
  void InitPrefChangeRegistrars();

  // Called when a data provider pref changes.
  void OnCalendarPrefChanged();
  void OnFileSuggestPrefChanged();
  void OnRecentTabPrefChanged();
  void OnWeatherPrefChanged();
  void OnReleaseNotesPrefChanged();

  // Records metrics on which providers are hidden based on prefs.
  void RecordProviderHiddenHistograms();

  // Whether `item_remover_` is created and initialized.
  bool IsItemRemoverInitialized();

  // Whether this is a post-login fetch (occurring right after login).
  bool is_post_login_fetch_ = false;

  // Whether the calendar event data is freshly fetched.
  bool is_calendar_data_fresh_ = false;

  // Whether the calendar event attachment data is freshly fetched. In practice
  // this should mirror `is_calendar_data_fresh_` but it makes the code more
  // consistent to track this separately.
  bool is_attachment_data_fresh_ = false;

  // Whether the current files data is freshly fetched.
  bool is_files_data_fresh_ = false;

  // Whether the current tabs data is freshly fetched.
  bool is_tabs_data_fresh_ = false;

  // Whether the current weather data is freshly fetched.
  // TODO(323229328): Use a timestamp to determine if weather is fresh.
  bool is_weather_data_fresh_ = false;

  // Whether the current release notes data is freshly fetched.
  bool is_release_notes_data_fresh_ = false;

  size_t next_request_id_ = 0u;
  // Pending data fetched requests mapped by their request IDs. IDs are
  // generated by incrementing `next_request_id_`.
  std::map<size_t, PendingRequest> pending_requests_;

  // When the last fetch was started. Used for metrics.
  base::Time fetch_start_time_;

  // Which fetches are in progress. Used for metrics.
  bool is_fetching_calendar_ = false;
  bool is_fetching_attachment_ = false;
  bool is_fetching_file_suggest_ = false;
  bool is_fetching_recent_tab_ = false;
  bool is_fetching_weather_ = false;
  bool is_fetching_release_notes_ = false;

  // A type-specific list of calendar event items.
  std::vector<BirchCalendarItem> calendar_items_;

  // A type-specific list of calendar event attachment items.
  std::vector<BirchAttachmentItem> attachment_items_;

  // A type-specific list of items for all file suggestion items.
  std::vector<BirchFileItem> file_suggest_items_;

  // A type-specific list of items for all tab items.
  std::vector<BirchTabItem> recent_tab_items_;

  // A type-specific list of weather items.
  std::vector<BirchWeatherItem> weather_items_;

  // A type-specific list of release notes items.
  std::vector<BirchReleaseNotesItem> release_notes_items_;

  raw_ptr<BirchClient> birch_client_ = nullptr;

  std::unique_ptr<BirchDataProvider> weather_provider_;

  // When set, this clock is used to ensure a consistent current time is used
  // for testing.
  raw_ptr<base::Clock> clock_override_ = nullptr;

  // Whether an active user session changed notification has been seen. Used to
  // detect the initial notification on signin.
  bool has_active_user_session_changed_ = false;

  PrefChangeRegistrar calendar_pref_registrar_;
  PrefChangeRegistrar file_suggest_pref_registrar_;
  PrefChangeRegistrar recent_tab_pref_registrar_;
  PrefChangeRegistrar weather_pref_registrar_;
  PrefChangeRegistrar release_notes_pref_registrar_;

  // Used to filter out items which have previously been removed by the user.
  std::unique_ptr<BirchItemRemover> item_remover_;

  // A list of current BirchModel::Observers.
  base::ObserverList<Observer> observers_;
};

}  // namespace ash

#endif  // ASH_BIRCH_BIRCH_MODEL_H_
