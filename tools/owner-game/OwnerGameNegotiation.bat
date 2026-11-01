@echo off
setlocal
set "OGDIR=%APPDATA%\cm0102"
set "CONTROL=%OGDIR%\owner-game-negotiation-control.txt"
if not exist "%OGDIR%" mkdir "%OGDIR%" >nul 2>&1

title Owner Game - Negotiation Room

:menu
cls
echo ==============================================================
echo                 OWNER GAME - NEGOTIATION ROOM
echo ==============================================================
echo.
echo Figo / Aston Villa proof negotiation
echo CM still decides the player's demand and final answer.
echo.
echo   1  Hold position          - keep wage authority at GBP120k/week
echo   2  Approve exception      - allow up to GBP160k/week for this deal
echo   3  Take time              - mark the negotiation dormant
echo   4  Final offer            - mark final offer + allow GBP160k/week
echo   5  Walk away              - mark the deal walked away
echo   6  Reopen                 - reopen, hold at GBP120k/week
echo   7  Reopen + exception     - reopen with GBP160k/week authority
echo   8  Reset                  - active, no owner exception
echo   Q  Quit
echo.
choice /c 12345678Q /n /m "Choose an owner action: "

if errorlevel 9 goto :eof
if errorlevel 8 goto reset
if errorlevel 7 goto reopenexception
if errorlevel 6 goto reopen
if errorlevel 5 goto walk
if errorlevel 4 goto final
if errorlevel 3 goto time
if errorlevel 2 goto exception
if errorlevel 1 goto hold

:hold
>"%CONTROL%" echo active_hold
goto applied

:exception
>"%CONTROL%" echo active_exception
goto applied

:time
>"%CONTROL%" echo dormant
goto applied

:final
>"%CONTROL%" echo final_offer_exception
goto applied

:walk
>"%CONTROL%" echo walked_away
goto applied

:reopen
>"%CONTROL%" echo reopened_hold
goto applied

:reopenexception
>"%CONTROL%" echo reopened_exception
goto applied

:reset
>"%CONTROL%" echo active_hold
goto applied

:applied
cls
echo ==============================================================
echo                     OWNER ACTION QUEUED
echo ==============================================================
echo.
type "%CONTROL%"
echo.
echo Return to CM01/02 and open or re-open Figo's Negotiate screen.
echo The Owner Game state is applied when CM builds that contract screen.
echo.
echo A diagnostic file will be written to:
echo %OGDIR%\owner-game-negotiation.json
echo.
pause
goto menu
