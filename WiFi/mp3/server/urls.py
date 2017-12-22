
from django.conf.urls import url, include
from django.contrib import admin
from django.views.generic import TemplateView
from server.views import Play, Pause, Previous, Next, Fastforward, Shuffle, AboutView, volumeUp, volumeDown

urlpatterns =[
    url(r'^$', AboutView.as_view(), name='home'),
    url(r'^play/', Play.as_view(), name="play"),
    url(r'^pause/', Pause.as_view(), name="pause"),
    url(r'^previous/', Previous.as_view(), name="previous"),
    url(r'^next/', Next.as_view(), name="next"),
    url(r'^fastforward/', Fastforward.as_view(), name="fastforward"),
    url(r'^shuffle/', Shuffle.as_view(), name="shuffle"),
    url(r'^volUP/', volumeUp.as_view(), name="volUP"),
    url(r'^volDOWN/', volumeDown.as_view(), name="volDOWN"),
]
