import ndspy.bmg

message1 = ndspy.bmg.Message(b'', ['For better performance,\nif possible please run the\ngame in DSi CPU mode.'])

bmg = ndspy.bmg.BMG.fromMessages([message1])

bmg.encoding = 'utf-16'
bmg.saveToFile('02_infotextbox.bmg')
