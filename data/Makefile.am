
DISTCLEANFILES =
EXTRA_DIST = \
	mobile-radio-monitor.desktop \
	mobile-radio-monitor.png

desktopdir = $(datadir)/applications
desktop_DATA = mobile-radio-monitor.desktop

# NOTE: yes, this is gnome-system-monitor's icon with other colors, yes.
# All credits go to jimmac, who I believe is the one who draw this one.
icon256dir = $(datadir)/icons/hicolor/256x256/apps
icon256_DATA = mobile-radio-monitor.png

# Setup policy
edit = @sed -e 's|@bindir[@]|$(bindir)|g'
es.aleksander.MobileRadioMonitor.pkexec.policy: es.aleksander.MobileRadioMonitor.pkexec.policy.in
	$(edit) $< >$@

polkit_policydir    = $(datadir)/polkit-1/actions
polkit_policy_DATA  = es.aleksander.MobileRadioMonitor.pkexec.policy
DISTCLEANFILES     += es.aleksander.MobileRadioMonitor.pkexec.policy
EXTRA_DIST         += es.aleksander.MobileRadioMonitor.pkexec.policy.in
