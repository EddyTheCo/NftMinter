import QtQuick.Controls

import Esterv.Utils.Updater


Frame
{
    visible: Updater.state&&Updater.hasUpdate
	UpdateBox
	{
		anchors.fill: parent
	}
}
