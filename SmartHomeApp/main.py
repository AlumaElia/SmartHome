from kivy.app import App
from kivymd.app import MDApp
from kivy.lang import Builder
from kivy.uix.screenmanager import ScreenManager, Screen
from kivy.properties import NumericProperty, ObjectProperty, StringProperty
from kivy.uix.popup import Popup
from kivy.uix.label import Label
from database import DataBase
from kivy.core.window import Window
from kivymd.uix.textfield import MDTextField
from kivymd.uix.button import MDRoundFlatButton
from kivymd.uix.button import MDIconButton
from kivy.event import EventDispatcher
from mqtt_p import Mqtt_service


class MyEventDispatcher(EventDispatcher):

    def __init__(self, **kwargs):
        # self.register_event_type('on_loginBtn')
        self.register_event_type('on_light')
        self.register_event_type('on_lightUI_button')
        self.register_event_type('on_boiler')
        self.register_event_type('on_show_temp')
        self.register_event_type('on_boilerUI_button')
        super(MyEventDispatcher, self).__init__(**kwargs)

    def light_update(self, status):
        # when light_status is called, the 'on_light' event will be
        # dispatched with the value
        self.dispatch('on_light', status)

    def on_light(self, *args):
        print("massage sent to the broker", args[0])

    def lightUI_button(self, status, msg):
        self.dispatch('on_lightUI_button', status)

    def on_lightUI_button(self, *args):
        print("update sent to the ui", args)
        if args[0] == "On":
            App.get_running_app().root.screens[2].ids.light_text.text = args[0]
            App.get_running_app().root.screens[2].ids.light_icon.icon = "lightbulb-on-outline"

        if args[0] == "Off":
            App.get_running_app().root.screens[2].ids.light_text.text = args[0]
            App.get_running_app().root.screens[2].ids.light_icon.icon = "lightbulb-outline"


    def boiler_update(self, status):
        self.dispatch('on_boiler', status)

    def on_boiler(self, *args):
        print("massage sent to the broker", args)

    def boilerUI_button(self, status):
        self.dispatch('on_boilerUI_button', status)

    def on_boilerUI_button(self, *args):
        print("update sent to the ui", args)
        if args[0] == "On":
            App.get_running_app().root.screens[3].ids.boiler_text.text = args[0]

        if args[0] == "Off":
            App.get_running_app().root.screens[3].ids.boiler_text.text = args[0]

    def show_temp(self, temp):
        self.dispatch('on_show_temp', temp)

    def on_show_temp(self, temp):
        App.get_running_app().root.screens[3].val = str(temp)
        App.get_running_app().root.screens[3].scale = self.fit_to_scale(temp, 15, 70)

    def fit_to_scale(self, value, minimum, maximum):
        # scales the given value between the set max and min limits
        return ((value - minimum) / (maximum - minimum)) * 300 - 150


class CreateAccountWindow(Screen):
    namee = ObjectProperty(None)
    email = ObjectProperty(None)
    password = ObjectProperty(None)

    def submit(self):
        if self.namee.text != "" and self.email.text != "" and self.email.text.count(
                "@") == 1 and self.email.text.count(".") > 0:
            if self.password.text != "":
                if db.add_user(self.email.text, self.password.text, self.namee.text) == 1:
                    self.reset()
                    sm.current = "login"
                else:
                    self.emailExists()
            else:
                invalidForm()
        else:
            invalidForm()

    def login(self):
        self.reset()
        sm.current = "login"

    def reset(self):
        self.email.text = ""
        self.password.text = ""
        self.namee.text = ""

    def emailExists(self):
        pop = Popup(title='Email exists',
                    content=Label(text='Email exists already,\n go  back to login or try again'),
                    size_hint=(0.8, 0.2))
        pop.open()

class LoginWindow(Screen):
    email = ObjectProperty(None)
    password = ObjectProperty(None)

    def loginBtn(self):
        if db.validate(self.email.text, self.password.text):
            ControlWindow.current = self.email.text
            self.reset()
            sm.current = "control"
        else:
            invalidLogin()
        # ev.loginBtn()

    def createBtn(self):
        self.reset()
        sm.current = "create"

    def reset(self):
        self.email.text = ""
        self.password.text = ""


class ControlWindow(Screen):
    n = ObjectProperty(None)
    created = ObjectProperty(None)
    email = ObjectProperty(None)
    light_text = ObjectProperty(None)
    light_icon = ObjectProperty(None)
    current = ""

    def logOut(self):
        sm.current = "login"

    def on_enter(self, *args):
        password, name, created = db.get_user(self.current)
        self.n.text = "Welcome Home " + name

    def light_pressed(self):  # handle publish

        if MyMainApp.light_s == "On":
            ev.light_update("On")

        if MyMainApp.light_s == "Off":
            ev.light_update("Off")

    def unavailable(self):
        pop = Popup(title='unavailable function',
                    content=Label(text='this function is unavailable for now.'),
                    size_hint=(0.8, 0.2))

        pop.open()

class BathRoom(Screen):
    numOfShowers = ObjectProperty(None)
    boiler_text = ObjectProperty(None)
    boiler_icon = ObjectProperty(None)
    scale = NumericProperty(150)
    val = StringProperty('--')
    unit = StringProperty('°C')

    def boiler_pressed(self):
        if MyMainApp.boiler_s == "On":
            ev.boiler_update('On')

        if MyMainApp.boiler_s == "Off":
            ev.boiler_update('Off')


class WindowManager(ScreenManager):
    pass


def invalidLogin():
    pop = Popup(title='Invalid Login',
                content=Label(text='Invalid username or password.'),
                size_hint=(0.8, 0.2))
    pop.open()


def invalidForm():
    pop = Popup(title='Invalid Form',
                content=Label(text='Please fill in all inputs \nwith valid information.'),
                size_hint=(0.8, 0.2))

    pop.open()



kv = Builder.load_file("my.kv")
sm = WindowManager()
db = DataBase("users.txt")
ev = MyEventDispatcher()

class MyMainApp(MDApp):
    service = None
    light_s = "Off"
    boiler_s = "Off"

    def build(self):
        # self.theme_cls.theme_style = "Dark"
        self.theme_cls.primary_palette = "Brown"
        self.theme_cls.primary_hue = "700"
        screens = [LoginWindow(name="login"), CreateAccountWindow(name="create"),
                   ControlWindow(name="control"), BathRoom(name="bathroom")]

        for screen in screens:
            sm.add_widget(screen)

        self.service = Mqtt_service()
        # ev.bind(on_loginBtn=self.connection)
        ev.bind(on_light=self.my_callback_light)
        ev.bind(on_boiler=self.my_callback_boiler)
        # self.service.connection_failed = self.connection_failed
        self.service.updateUI_light = self.updateUI_light
        self.service.updateUI_temp = self.updateUI_temp
        self.service.updateUI_boiler = self.updateUI_boiler

        sm.current = "login"

        return sm

    def my_callback_light(self, value, *args):
        self.service.publishMsg_light(args[0])

    def updateUI_light(self, status, msg):
        self.light_s = status
        ev.lightUI_button(status, msg)
        print("Main: " + msg)

    def my_callback_boiler(self, value, *args):
        num = self.root.screens[3].ids.numOfShowers.text
        self.service.publishMsg_boiler(num, args[0])

    def updateUI_boiler(self, status):
        self.boiler_s = status
        ev.boilerUI_button(status)
        print(status)

    def updateUI_temp(self, temp):
        ev.show_temp(temp)
        print(temp)


#ad can

if __name__ == "__main__":
    MyMainApp().run()
