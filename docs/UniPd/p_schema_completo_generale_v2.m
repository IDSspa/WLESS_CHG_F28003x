 
clear all;
close all;
clc;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                   %
%                     Parametri del sistema                         %
%                                                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Si definiscono i parametri di funzionamento con valori uguali per la sezione
% sezione trasmittente e la sezione ricevente.


%%%%%%%%%%%%%
% Batterie
%%%%%%%%%%%%%

% Estremi di variazione della tensione della batteria.

% Limiti di tensione della batteria 
V_bat_nom=96;            % Tensione nominale
V_bat_min=75;            % Tensione minima (batteria scarica)
V_bat_max=109;           % Tensione massiam (batteria carica)



% Limiti corrente batteria (la corrente è considerata positiva quando esce dalla batteria)
Ibat_rif_max=50;         % Massima corrente di scarica della batteria
Ibat_rif_min=-50;        % Massima corrente di carica della batteria 


% A seconda del tipo di simulazione effettuata, la batteria può essere modellata
% come un generatore di tensione costante oppure come un condensatore molto 
% grande in modo che la sua tensione sia lentamente variabile nel corso
% della simulazione.

R_bat=32e-3;             % Resistenza interna della batteria. Ricavata da quella di un modulo a 50V, 100 A che risulta di 8 mOhm

C_bat_eq_tx=3.5;%0.25;   % Condensatore equivalente usato per rappresentare la batteria
C_bat_eq_rx=1.0;         % Sono usati valori diversi per le due sezioni in modo da simulare la condizione per cui una
%C_bat_eq=0.25;           % batteria si scarica prima che l'altra sia carica o viceversa 

                                             


%%%%%%%%%%%%%%%%%%%%%%
% Convertitori statici
%%%%%%%%%%%%%%%%%%%%%%

% Convertitore dc-ac (invertitore)
Rds_on_dc_ac=1e-3;    % Resistenza di conduzione degli interruttori statici che costituiscono il convertitore dc-ac
Ron_diodo_dc_ac=1e-3; % Resistenza di conduzione dei diodi posti in parallelo agli interruttori statici
Vf_diodo_dc_ac=0.7;   % Caduta di tensione sui diodi in conduzione
Rsnubber_dc_ac=inf;   % Resistenze di snubber del convertitore dc-dc (serve solo per la simulazione)
Csnubber_dc_ac=inf;   % Capacità di snubber del convertitore dc-dc (serve solo per la simulazione)
freq_dc_ac=85e3;      % Frequenza di switching del convertitore dc-ac della sezione trasmittente.
                      % Corrisponde alla frequenza della corrente nella bobina trasmittente
freq_dc_ac_nom=85e3;  % Frequenza di switching nominale del convertitore dc-ac della sezione trasmittente.                            
T_dc_ac=1/freq_dc_ac; % Periodo di switching del convertitore dc-ac
dead_time_dc_ac=0.3e-6;  % Dead time 

w_dc_ac_nom=2*pi*freq_dc_ac_nom; % Pulsazione di alimentazione nominale della bobina lato trasmittente

% Convertitore dc-dc (chopper interleaved)
Rds_on_dc_dc=1e-3;    % Resistenza di conduzione degli interruttori statici che costituiscono il convertitore dc-dc (chopper)
Ron_diodo_dc_dc=1e-3; % Resistenza di conduzione dei diodi posti in parallelo agli interruttori statici
Vf_diodo_dc_dc=0.7;   % Caduta di tensione sui diodi in conduzione
Rsnubber_dc_dc=inf;   % Resistenze di snubber del convertitore dc-dc (serve solo per la simulazione)
Csnubber_dc_dc=inf;   % Capacità di snubber del convertitore dc-dc (serve solo per la simulazione)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%freq_dc_dc_tx=120e3;     % Frequenza di switching del convertitore dc-dc della sezione trasmittente
freq_dc_dc=3/2*freq_dc_ac;     % (127.5 kHz) Frequenza di switching del convertitore dc-dc della sezione trasmittente
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
T_dc_dc=1/freq_dc_dc;    % Periodo di switching del convertitore dc-dc

dead_time_dc_dc=0.3e-6;  % Dead time del convertitore dc-dc della sezione trasmittente

L_filtro_dc_dc=65e-6;    % Induttanza di filtro di ognuno dei rami del convertitore dc-dc.
R_Lfiltro_dc_dc=6.5e-3;  % Resistenza parassita dell'induttanza di filtro
Rshunt_Lfiltro=1e6;      % Resistenza di shunt in parallelo all'induttanza di filtro (serve solo per simulazione)

C_bus_dc_dc=1.5e-3;      % Condensatore di filtro sul bus dc. 
V_dc_nom=250;            % Tensione nominale del bus dc


%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Bobine di accoppiamento
%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Bobina lato trasmittente
L_coil=54e-6;                                % Auto induttanza
Q_coil=100;                                  % Fattore di merito dell bobine (ipotesi per fissare la resistenza parassita)
R_coil=w_dc_ac_nom*L_coil/Q_coil;            % Resistenza parassita della bobina trasmittente
C_ris=1/(w_dc_ac_nom^2*L_coil);              % Capacità del condensatore di risonanza alla pulsazione nominale 
Rshunt_coil=1e5;                             % Resistenza di shunt in parallelo alle bobine (serve solo per simulazione)

% Coefficiente di accoppiamento (ipotesi per fissare la mutua induttanza)
k=0.23;

M_coil=k*sqrt(L_coil^2);                     % Mutua induttanza
R_coil_M=sqrt(w_dc_ac_nom^2)*M_coil/sqrt(Q_coil^2);

Icoil_amp_max=40;                            % Massima ampiezza della corrente nella bobina
Icoil_amp_min=0;                             % Minima ampiezza della corrente nella bobina (essendo una ampiezza è positiva)


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                         %
%                    Ritardi di campionamento e filtri                    %
%                                                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


% Filtro anti-aliasing
% Filtro passa basso presente all'uscita degli inseguitori di tensione
R_filtro_anti_aliasing=19.6;
C_filtro_anti_aliasing=0.1e-6;
tau_filtro_anti_aliasing=R_filtro_anti_aliasing*C_filtro_anti_aliasing;
f_filtro_anti_aliasing=1/(2*pi*tau_filtro_anti_aliasing);
filtro_anti_aliasing_tf=tf(1,[tau_filtro_anti_aliasing 1]);


% Periodo di campionamento del controllore del convertitore dc/ac
Ts_dc_ac=4*T_dc_ac;     % Quattro volte il periodo di commutazione del convertitore dc/ac
                        % Come nel prototipo in laboratorio a Padova
                        % Si suppone sia uguale sui due lati del sistema

% Periodo di campionamento del controllore del convertitore dc/dc
Ts_dc_dc=Ts_dc_ac;      % Uguale al periodo di campionamento del convertitore dc/ac
                      


% Periodo di aggiornamento dati nella trasmissione tra le due sezioni del WPTS
Ts_trasmissione=1e-3;

% Periodo di aggiornamento dati nella trasmissione dal BMS
Ts_BMS=1e-3;

% Filtro passa basso per determinazione del valore medio della ampiezza della corrente nelle bobine
f_filtro_pb_coil=5e3;
tau_filtro_pb_coil=1/(2*pi*f_filtro_pb_coil);
filtro_pb_coil_tf=tf(1,[tau_filtro_pb_coil 1]);

% Modello del Ritardo di campionamento del convertitore dc/dc
num_rit_camp_dc_dc=[-Ts_dc_dc 1];
den_rit_camp_dc_dc=[Ts_dc_dc 1];
ritardo_di_campionamento_dc_dc_tf=tf(num_rit_camp_dc_dc,den_rit_camp_dc_dc);

% Modello del Ritardo di campionamento del convertitore dc/ac
num_rit_camp_dc_ac=[-Ts_dc_ac 1];
den_rit_camp_dc_ac=[Ts_dc_ac 1];
ritardo_di_campionamento_dc_ac_tf=tf(num_rit_camp_dc_ac,den_rit_camp_dc_ac);

% Modello del Ritardo di trasmissione
num_rit_trasm=[-Ts_trasmissione/2 1];
den_rit_trasm=[Ts_trasmissione/2 1];
ritardo_di_trasmissione_tf=tf(num_rit_trasm,den_rit_trasm);

% Frequenza di taglio del filtro passa basso per ottenere il valore medio
% della corrente (usato solo per la simulazione)
f_filtro_pb_corrente_bat=freq_dc_dc/20;
tau_filtro_pb_corrente_bat=1/(2*pi*f_filtro_pb_corrente_bat);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                   %
%                Condizioni iniziali del sistema                    %
%                                                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Lato trasmnittente

V_bat_0_tx=V_bat_max-5;%V_bat_nom;        % Tensione della batteria lato trasmettitore
V_dc_0_tx=220;%V_bat_0_tx;%250;      % Tensione del bus dc del convertitore dc-dc lato trasmettitore.
                              % E'aumentata rispetto ai 200 V ipotizzati inizialmente perchè altrimenti
                              % non è possibile trasferire la potenza richiesta

I_Lfiltro_A_0_tx=0;              % Corrente iniziale nell'induttore del ramo A del convertitore dc-dc
I_Lfiltro_B_0_tx=0;              % Corrente iniziale nell'induttore del ramo B del convertitore dc-dc


duty_A_0_tx=V_bat_0_tx/V_dc_0_tx;   % Valore iniziale del duty cycle nel ramo A del convertitore dc/dc 
duty_B_0_tx=duty_A_0_tx;            % Valore iniziale del duty cycle nel ramo B del convertitore dc/dc 

% Lato ricevente

V_bat_0_rx=V_bat_nom;        
V_dc_0_rx=230;%V_bat_0_rx;%V_dc_0_tx;                  

I_Lfiltro_A_0_rx=0;             
I_Lfiltro_B_0_rx=0;    

duty_A_0_rx=V_bat_0_rx/V_dc_0_rx;
duty_B_0_rx=duty_A_0_rx;


% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% % Valori utilizzati nel modello ai valori medi del convertitore dc-dc
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% % Si ipotizza che a regime le correnti nei due rami del convertitore dc-dc siano uguali e pari a metà della corrente di batteria.
% % Di conseguenza anche i duty cycle dei due rami del convertitore sono considerati uguali.
% 
% % Idealmente il duty cycle è duty = V_bat / V_dc.
% % Considerando la caduta di tensione sulla resistenza della bobina e sugli interruttori
% % si ottiene un valore leggermente diverso.
% %duty_leg_A_dc_dc_0_tx=V_bat_0_tx/V_dc_0_tx*(1+sqrt(1-(2*V_dc_0_tx*Req_Lfiltro_dc_dc_tx*I_coil_tx_0/V_bat_0_tx^2)))/2;
duty_leg_A_dc_dc_0_tx=V_bat_0_tx/V_dc_0_tx;
duty_leg_B_dc_dc_0_tx=duty_leg_A_dc_dc_0_tx; 
% 
% %duty_leg_A_dc_dc_0_rx=V_bat_0_rx/V_dc_0_rx*(1+sqrt(1-(2*V_dc_0_rx*Req_Lfiltro_dc_dc_rx*I_coil_rx_0/V_bat_0_rx^2)))/2;
duty_leg_A_dc_dc_0_rx=V_bat_0_rx/V_dc_0_rx;
duty_leg_B_dc_dc_0_rx=duty_leg_A_dc_dc_0_rx; 

I_bat_0_tx=I_Lfiltro_A_0_tx/duty_leg_A_dc_dc_0_tx+I_Lfiltro_B_0_tx/duty_leg_B_dc_dc_0_tx; % Corrente nella batteria
I_bat_0_rx=I_Lfiltro_A_0_rx/duty_leg_A_dc_dc_0_rx+I_Lfiltro_B_0_rx/duty_leg_B_dc_dc_0_rx; % Corrente nella batteria



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                 %
%              Limiti di funzionamento del sistema                %
%                                                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Riferimenti di tensione per le batterie e i bus dc durante i trasferimenti di potenza 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Lato trasmittente
Vbat_rif_min_tx=V_bat_min;  % Riferimento per la tensione minima della batteria. Usato nel trasferimento da trasmetitore  ricevitore
Vbat_rif_max_tx=V_bat_max;  % Riferimento per la tensione massima della batteria. Usato nel trasferimento da trasmetitore  ricevitore
Vdc_rif_min_tx=V_dc_nom-5;  % Riferimento per la tensione minima del bus dc lato trasmettitore. E' usato dagli anelli di controllo che scaricano il condensatore
Vdc_rif_max_tx=V_dc_nom+5;  % Riferimento per la tensione massima del bus dc lato trasmettitore. E' usato dagli anelli di controllo che caricano il condensatore

% Lato ricevente
Vbat_rif_min_rx=V_bat_min;
Vbat_rif_max_rx=V_bat_max;
Vdc_rif_min_rx=V_dc_nom-5;
Vdc_rif_max_rx=V_dc_nom+5;


V_dc_min_alg=V_bat_min/4;     % Minima tensione Vdc per cui gli algoritmi di controllo funzionano ancora
                              % Viene definita questa tensione minima perchè in alcuni calcoli Vdc compare
                              % al denominatore e bisogna evitare le divisioni per 0
V_dc_max_alg=V_bat_max*5;     % Massima tensione Vdc per cui gli algoritmi di controllo funzionano ancora
V_bat_min_alg=V_bat_min/2;    % Massima tensione di batteria per cui gli algoritmi di controllo funzionano ancora
V_bat_max_alg=V_bat_max*2;    % Minima tensione di batteria per cui gli algoritmi di controllo funzionano ancora



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                        %
%                Controllo della corrente di batteria                    %
%                                                                        %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Progettazione dell'algoritmo di controllo del convertitore dc-dc (chopper)
% La grandezza controllata è la corrente che fluisce nella bobina di filtro
% di un ramo del convertitore. Essa corrisponde a metà della corrente di
% batteria. Questo anello di controllo è attivo sia durante la carica che
% la scarica delle batterie.
% Il riferimento di corrente della batteria è calcolato a partire da un
% riferimento si potenza. Si opera in questo modo perchè l'anello di
% controllo in esame dovrà operare come anello interno dei due anelli di
% controllo che regolano la tensione della batteria e la tensione del bus
% dc.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Specifiche per l'anello di controllo
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
banda_passante_corrente_batteria_Hz=250;
margine_fase_corrente_batteria_deg=75;

banda_passante_corrente_batteria=banda_passante_corrente_batteria_Hz*2*pi;
margine_fase_corrente_batteria=margine_fase_corrente_batteria_deg*pi/180;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Sistema da controllare
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Il sistema da controllare è costituito dalla batteria (considerata un
% generatore di tensione) e dalla induttanza di filtro di un ramo dell'invertitore dc-dc
% In retroazione c'è il filtro passa basso anti aliasing

Req_Lfiltro_dc_dc=(R_Lfiltro_dc_dc+Rds_on_dc_dc); % Resistenza equivalente in serie all'induttanza di filtro.
                                                  % Tiene conto anche della resistenza di conduzione degli interruttori
tau_induttanza_filtro=L_filtro_dc_dc/Req_Lfiltro_dc_dc;

% Funzione di trasferimento dalla tensione ai capi della induttanza di
% filtro alla corrente nella induttanza di filtro. Tiene conto anche della
% resistenza parassita della batteria
induttanza_filtro_tf=tf(1/(Req_Lfiltro_dc_dc+R_bat),[tau_induttanza_filtro 1]);

% Funzione trasferimento a catena aperta induttanza + campionamento
induttanza_filtro_campionamento_tf=series(induttanza_filtro_tf,ritardo_di_campionamento_dc_dc_tf);

% Funzione trasferimento a catena aperta induttanza + campionamento + filtro anti aliasing
induttanza_filtro_campionamento_aliasing_tf=series(induttanza_filtro_campionamento_tf,filtro_anti_aliasing_tf);

[~,fase_indut_filtro_camp_deg,~]=bode(induttanza_filtro_campionamento_aliasing_tf,banda_passante_corrente_batteria);
fase_indut_filtro_camp=fase_indut_filtro_camp_deg*pi/180;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Progettazione del controllore PI
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Determinazione della costante di tempo del controllore PI
tau_PI_corrente_batteria=1/banda_passante_corrente_batteria*tan(-pi/2+margine_fase_corrente_batteria-fase_indut_filtro_camp); % Costante di tempo del PI di corrente

PI_corrente_batteria_k_1_tf=tf([tau_PI_corrente_batteria 1],[tau_PI_corrente_batteria 0]); % Funzione di traferimento PI con guadagno proporzionale unitario

catena_aperta_anello_corrente_batteria_k_1_tf=series(PI_corrente_batteria_k_1_tf,induttanza_filtro_campionamento_aliasing_tf);
% Guadagno a catena aperta alla frequenza di taglio con controllore controllore avente guadagno proporzionale unitario
[modulo_catena_aperta_corrente_batteria,~,~]=bode(catena_aperta_anello_corrente_batteria_k_1_tf,banda_passante_corrente_batteria); 
Kp_PI_corrente_batteria=1/modulo_catena_aperta_corrente_batteria;          % Guadagno proporzionale del controllore. Porta a 1 il guadagno a catena aperta.
Ki_PI_corrente_batteria=Kp_PI_corrente_batteria/tau_PI_corrente_batteria;  % Guadagno integrale del controllore.

disp('Controllo corrente batteria')
disp(' ');
disp(['Banda passante = ' num2str(banda_passante_corrente_batteria_Hz) ' Hz']);
disp(['Margine di fase = ' num2str(margine_fase_corrente_batteria_deg) ' °']); 
disp(['Kp anello di corrente batteria = ' num2str(Kp_PI_corrente_batteria)]);
disp(['Ki anello di corrente batteria = ' num2str(Ki_PI_corrente_batteria)]);

PI_corrente_batteria_tf=tf(Kp_PI_corrente_batteria*[tau_PI_corrente_batteria 1],[tau_PI_corrente_batteria 0]); % Funzione di traferimento PI

% Funzione di trasferimento ad anello aperto del sistema con regolatore
catena_aperta_anello_corrente_batteria_tf=series(PI_corrente_batteria_tf,induttanza_filtro_campionamento_aliasing_tf);

% Funzione di trasferimento ad anello chiuso del sistema con regolatore
catena_diretta_anello_corrente_batteria_tf=series(PI_corrente_batteria_tf,induttanza_filtro_campionamento_tf);
catena_chiusa_anello_corrente_batteria_tf=feedback(catena_diretta_anello_corrente_batteria_tf,filtro_anti_aliasing_tf,-1);

% figure(1)
% bode(induttanza_filtro_campionamento_aliasing_tf);
% hold on
% bode(induttanza_filtro_campionamento_aliasing_tf,banda_passante_corrente_batteria,'o');
% grid on
% hold off
% 
% figure(2)
% bode(catena_aperta_anello_corrente_batteria_tf);
% hold on
% bode(catena_aperta_anello_corrente_batteria_tf,banda_passante_corrente_batteria,'o');
% bode(catena_chiusa_anello_corrente_batteria_tf);
% grid on
% hold off
% 
% figure(3)
% step(catena_chiusa_anello_corrente_batteria_tf);
% grid on

%%%%%%%%%%%%%%%%%%%%%%%%%%
% Discretizzazione del PI
%%%%%%%%%%%%%%%%%%%%%%%%%%
disp(' ');
disp('Controllore corrente di batteria discreto')
PI_corrente_batteria_tf_disc=c2d(PI_corrente_batteria_tf,Ts_dc_dc,'tustin');
PI_corrente_batteria_tf_disc_num=PI_corrente_batteria_tf_disc.Numerator{1};
PI_corrente_batteria_tf_disc_den=-PI_corrente_batteria_tf_disc.Denominator{1};
disp(['Rif(k) = ' num2str(PI_corrente_batteria_tf_disc_den(2)) ' Rif(k-1) + ' num2str(PI_corrente_batteria_tf_disc_num(1)) ' Err(k) + '...
    num2str(PI_corrente_batteria_tf_disc_num(2)) ' Err(k-1)']);

disp(' ');
disp(' ');

Parametri_Controllori=[PI_corrente_batteria_tf_disc_den(2) PI_corrente_batteria_tf_disc_num(1) PI_corrente_batteria_tf_disc_num(2)];


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                        %
%              Regolazione della tensione di batteria                    %
%                                                                        %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Regolazione della tensione di batteria mediante scambio di potenza con
% il condensatore del lato dc.
% Simula il funzionamento del BMS. Dal BMS si ottengono i limiti di
% corrente, non i riferimenti di corrente.
% Questo regolatore fornisce la potenza che dovrebbe essere iniettata o
% estratta dalla batteria per evitare di caricarla o scaricarla troppo.
% Se la potenza effettivamente scambiata è all'interno dei limiti la
% la tensione di batteria rimane nei limiti corretti.
% Si tratta di un anello di controllo esterno a quello di corrente.
% Nella simulazione la batteria è rappresentata come un condensatore di
% elevata capacità al fine di potere effettivamente effettuare la carica/scarica.
% Il regolatore è progettato supponendo di manipolare la potenza iniettata
% nel condensatore usato per rappresentare la batteria.
% La variabile controllata è la tensione al quadrato.
% L'impianto da regolare, su cui è dimensionato il regolatore, è costituito
% dalla FdT dell'anello di corrente e da 2/(s Cbat_eq).
% Sulle due sezioni del sistema WPT è possibile utilizzare valori diversi
% del condensatore che simula la batteria. Quindi bisogna dimensionare due
% regolatori di tensione.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Specifiche per l'anello di controllo
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

banda_passante_tensione_batteria_Hz=1;
margine_fase_tensione_batteria_deg=85;

banda_passante_tensione_batteria=banda_passante_tensione_batteria_Hz*2*pi;
margine_fase_tensione_batteria=margine_fase_tensione_batteria_deg*pi/180;


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Sistema da controllare
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Il sistema da controllare è costituito dalla batteria (modellata mediante un
% condensatore con una resistenza in serie) e dall'anello di corrente. 
% Non viene inserito un ritardo di campionamento perchè l'algoritmo di
% controllo viene implementato nello stesso ciclo di controllo 
% dell'anello di corrente interno. Il campionamento delle correnti del convertitore dc-dc
% e delle tensioni di batteria e del bus dc è effettuato nello stesso
% momento.


% Funzione di trasferimento dell'anello di corrente
anello_di_corrente_tf=catena_chiusa_anello_corrente_batteria_tf; % Calcolata nel paragrafo relativo al controllo di corrente;

% Funzione di trasferimento 2/(sC)
due_su_sCbat_tf_tx=tf(2,[C_bat_eq_tx 0]);
due_su_sCbat_tf_rx=tf(2,[C_bat_eq_rx 0]);


% Funzione di trasferimento catena diretta
batteria_anello_corrente_tf_tx=series(anello_di_corrente_tf,due_su_sCbat_tf_tx);
batteria_anello_corrente_tf_rx=series(anello_di_corrente_tf,due_su_sCbat_tf_rx);

% Funzione trasferimento a catena aperta batteria + anello di corrente + filtro anti aliasing
batteria_anello_corrente_aliasing_tf_tx=series(batteria_anello_corrente_tf_tx,filtro_anti_aliasing_tf);
batteria_anello_corrente_aliasing_tf_rx=series(batteria_anello_corrente_tf_rx,filtro_anti_aliasing_tf);



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Progettazione del regolatore di tipo P lato Tx
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

P_tensione_batteria_k_1_tf_tx=tf(1,1); % Funzione di traferimento P con guadagno proporzionale unitario

catena_aperta_anello_tensione_batteria_reg_P_k_1_tf_tx=series(P_tensione_batteria_k_1_tf_tx,batteria_anello_corrente_aliasing_tf_tx);

% Guadagno a catena aperta alla frequenza di taglio con controllore P avente guadagno proporzionale unitario
[modulo_catena_aperta_tensione_batteria_reg_P_tx,~,~]=bode(catena_aperta_anello_tensione_batteria_reg_P_k_1_tf_tx,banda_passante_tensione_batteria);
Kp_P_tensione_batteria_tx=1/modulo_catena_aperta_tensione_batteria_reg_P_tx;     % Guadagno proporzionale del regolatore. Porta a 1 il guadagno a catena aperta.

P_tensione_batteria_tf_tx=tf(Kp_P_tensione_batteria_tx,1); % Funzione di traferimento P

% Funzione di trasferimento ad anello aperto con regolatore P
catena_aperta_anello_tensione_batteria_reg_P_tf_tx=series(P_tensione_batteria_tf_tx,batteria_anello_corrente_aliasing_tf_tx);

% Funzione di trasferimento ad anello chiuso con regolatore P
catena_diretta_anello_tensione_batteria_reg_P_tf_tx=series(P_tensione_batteria_tf_tx,batteria_anello_corrente_tf_tx);
catena_chiusa_anello_tensione_batteria_reg_P_tf_tx=feedback(catena_diretta_anello_tensione_batteria_reg_P_tf_tx,filtro_anti_aliasing_tf,-1);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Progettazione del regolatore di tipo P lato Rx
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

P_tensione_batteria_k_1_tf_rx=tf(1,1); % Funzione di traferimento P con guadagno proporzionale unitario

catena_aperta_anello_tensione_batteria_reg_P_k_1_tf_rx=series(P_tensione_batteria_k_1_tf_rx,batteria_anello_corrente_aliasing_tf_rx);

% Guadagno a catena aperta alla frequenza di taglio con controllore P avente guadagno proporzionale unitario
[modulo_catena_aperta_tensione_batteria_reg_P_rx,~,~]=bode(catena_aperta_anello_tensione_batteria_reg_P_k_1_tf_rx,banda_passante_tensione_batteria);
Kp_P_tensione_batteria_rx=1/modulo_catena_aperta_tensione_batteria_reg_P_rx;     % Guadagno proporzionale del regolatore. Porta a 1 il guadagno a catena aperta.


P_tensione_batteria_tf_rx=tf(Kp_P_tensione_batteria_rx,1); % Funzione di traferimento P

% Funzione di trasferimento ad anello aperto con regolatore P
catena_aperta_anello_tensione_batteria_reg_P_tf_rx=series(P_tensione_batteria_tf_rx,batteria_anello_corrente_aliasing_tf_rx);

% Funzione di trasferimento ad anello chiuso con regolatore P
catena_diretta_anello_tensione_batteria_reg_P_tf_rx=series(P_tensione_batteria_tf_tx,batteria_anello_corrente_tf_rx);
catena_chiusa_anello_tensione_batteria_reg_P_tf_rx=feedback(catena_diretta_anello_tensione_batteria_reg_P_tf_rx,filtro_anti_aliasing_tf,-1);

disp('Controllo tensione batteria (solo per la simulazione)')
disp('Riproduce il comportamento del BMS');
disp(' ');
disp(['Kp anello di corrente batteria tx = ' num2str(Kp_P_tensione_batteria_tx)]);
disp(['Kp anello di corrente batteria rx = ' num2str(Kp_P_tensione_batteria_rx)]);

% figure(1)
% bode(batteria_anello_corrente_aliasing_tf_tx);
% hold on
% bode(batteria_anello_corrente_aliasing_tf_rx);
% bode(batteria_anello_corrente_aliasing_tf_tx,banda_passante_tensione_batteria,'o');
% bode(batteria_anello_corrente_aliasing_tf_rx,banda_passante_tensione_batteria,'o');
% grid on
% hold off
% 
% figure(2)
% bode(catena_aperta_anello_tensione_batteria_reg_P_tf_tx);
% hold on
% bode(catena_aperta_anello_tensione_batteria_reg_P_tf_rx);
% bode(catena_aperta_anello_tensione_batteria_reg_P_tf_tx,banda_passante_tensione_batteria,'o');
% bode(catena_aperta_anello_tensione_batteria_reg_P_tf_rx,banda_passante_tensione_batteria,'o');
% bode(catena_chiusa_anello_tensione_batteria_reg_P_tf_tx);
% bode(catena_chiusa_anello_tensione_batteria_reg_P_tf_rx);
% bode(catena_chiusa_anello_tensione_batteria_reg_P_tf_tx,banda_passante_tensione_batteria,'o');
% bode(catena_chiusa_anello_tensione_batteria_reg_P_tf_rx,banda_passante_tensione_batteria,'o');
% grid on
% hold off
% 
% figure(3)
% step(catena_chiusa_anello_tensione_batteria_reg_P_tf_tx);
% hold on
% step(catena_chiusa_anello_tensione_batteria_reg_P_tf_rx);
% grid on

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Discretizzazione dei regolatori
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Discretizzazione del regolatore P tx
P_tensione_batteria_tf_disc_tx=c2d(P_tensione_batteria_tf_tx,Ts_dc_dc,'tustin');
P_tensione_batteria_tf_disc_num_tx=P_tensione_batteria_tf_disc_tx.Numerator{1};
P_tensione_batteria_tf_disc_den_tx=-P_tensione_batteria_tf_disc_tx.Denominator{1};

% Discretizzazione del P
P_tensione_batteria_tf_disc_rx=c2d(P_tensione_batteria_tf_rx,Ts_dc_dc,'tustin');
P_tensione_batteria_tf_disc_num_rx=P_tensione_batteria_tf_disc_rx.Numerator{1};
P_tensione_batteria_tf_disc_den_rx=-P_tensione_batteria_tf_disc_rx.Denominator{1};

disp(' ');
disp('Controllore tensione di batteria discreto tx')
disp(['Rif(k) = ' num2str(P_tensione_batteria_tf_disc_num_tx(1)) ' Err(k)']);
disp('Controllore tensione di batteria discreto rx')
disp(['Rif(k) = ' num2str(P_tensione_batteria_tf_disc_num_rx(1)) ' Err(k)']);
disp(' ');
disp(' ');


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                        %
%                Regolazione della tensione del bus dc                   %
%      mediante scambio di potenza con la batteria dello stesso lato     %
%                                                                        %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Regolazione della tensione del bus dc mediante carica/scarica della batteria dello stesso lato
% Si tratta di un anello di controllo esterno a quello che controlla la corrente della batteria.
% Il regolatore è progettato supponendo di manipolare la potenza iniettata
% nel condensatore. La variabile controllata è la tensione al quadrato.
% L'impianto da regolare, su cui è dimensionato il regolatore, è costituito
% dalla FdT dell'anello di corrente e da 2/(s Cdc).
% La potenza scambiata è limitata ai valori estremi ottenuti simulando
% l'anello di controllo della tensione di batteria descritto sopra.

banda_passante_tensione_bus_dc_bat_Hz=10;
margine_fase_tensione_bus_dc_bat_deg=85;

banda_passante_tensione_bus_dc_bat=banda_passante_tensione_bus_dc_bat_Hz*2*pi;
margine_fase_tensione_bus_dc_bat=margine_fase_tensione_bus_dc_bat_deg*pi/180;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Sistema da controllare
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Il sistema da controllare è costituito dal condensatore del bus dc e dall'anello di corrente. 
% Non viene inserito un ritardo di campionamento perchè l'algoritmo di
% controllo viene implementato nello stesso ciclo di controllo 
% dell'anello di corrente interno. Il campionamento delle correnti del convertitore dc-dc
% e delle tensioni di batteria e del bus dc è effettuato nello stesso momento

% Funzione di trasferimento dell'anello di corrente
anello_di_corrente_tf=catena_chiusa_anello_corrente_batteria_tf; % Calcolata nel paragrafo relativo al controllo di corrente;

% Funzione di trasferimento 2/(sC)
due_su_sC_tf=tf(2,[C_bus_dc_dc 0]);

% Funzione di trasferimento complessiva
condensatore_dc_dc_anello_corrente_bat_tf=series(anello_di_corrente_tf,due_su_sC_tf);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Progettazione del regolatore di tipo PI
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Funzione trasferimento a catena aperta condensatore + anello di corrente + filtro anti aliasing
condensatore_dc_dc_anello_corrente_bat_aliasing_tf=series(condensatore_dc_dc_anello_corrente_bat_tf,filtro_anti_aliasing_tf);


[~,fase_tensione_bus_dc_deg,~]=bode(condensatore_dc_dc_anello_corrente_bat_aliasing_tf,banda_passante_tensione_bus_dc_bat);
fase_tensione_bus_dc=fase_tensione_bus_dc_deg*pi/180;

tau_PI_tensione_bus_dc_bat=1/banda_passante_tensione_bus_dc_bat*tan(-pi/2+margine_fase_tensione_bus_dc_bat-fase_tensione_bus_dc); % Costante di tempo del PI di tensione

PI_tensione_bus_dc_bat_k_1_tf=tf([tau_PI_tensione_bus_dc_bat 1],[tau_PI_tensione_bus_dc_bat 0]); % Funzione di traferimento PI con guadagno proporzionale unitario

catena_aperta_anello_tensione_bus_dc_bat_tf_k_1=series(PI_tensione_bus_dc_bat_k_1_tf,condensatore_dc_dc_anello_corrente_bat_aliasing_tf);
[modulo_catena_aperta_tensione_bus_dc_bat,~,~]=bode(catena_aperta_anello_tensione_bus_dc_bat_tf_k_1,banda_passante_tensione_bus_dc_bat);
Kp_PI_tensione_bus_dc_bat=1/modulo_catena_aperta_tensione_bus_dc_bat;
Ki_PI_tensione_bus_dc_bat=Kp_PI_tensione_bus_dc_bat/tau_PI_tensione_bus_dc_bat;

disp('Regolatore tensione del bus dc con scambio di potenza con la batteria dello stesso lato')
disp(' ');
disp(['Kp anello di tensione bus dc = ' num2str(Kp_PI_tensione_bus_dc_bat)]);
disp(['Ki anello di tensione bus dc = ' num2str(Ki_PI_tensione_bus_dc_bat)]);

PI_tensione_bus_dc_bat_tf=tf(Kp_PI_tensione_bus_dc_bat*[tau_PI_tensione_bus_dc_bat 1],[tau_PI_tensione_bus_dc_bat 0]); % Funzione di traferimento PI

% Funzione di trasferimento ad anello aperto
catena_aperta_tensione_bus_dc_bat_tf=series(PI_tensione_bus_dc_bat_tf,condensatore_dc_dc_anello_corrente_bat_aliasing_tf);

% Funzione di trasferimento ad anello chiuso
catena_diretta_tensione_bus_dc_bat_tf=series(PI_tensione_bus_dc_bat_tf,condensatore_dc_dc_anello_corrente_bat_tf);
catena_chiusa_tensione_bus_dc_bat_tf=feedback(catena_diretta_tensione_bus_dc_bat_tf,filtro_anti_aliasing_tf,-1);
 
% figure(1)
% bode(condensatore_dc_dc_anello_corrente_bat_tf);
% hold on
% bode(condensatore_dc_dc_anello_corrente_bat_tf,banda_passante_tensione_bus_dc_bat,'o');
% grid on
% hold off
% 
% figure(2)
% bode(catena_aperta_tensione_bus_dc_bat_tf);
% hold on
% bode(catena_aperta_tensione_bus_dc_bat_tf,banda_passante_tensione_bus_dc_bat,'o');
% bode(catena_chiusa_tensione_bus_dc_bat_tf);
% grid on
% hold off
% 
% figure(3)
% step(catena_chiusa_tensione_bus_dc_bat_tf);
% grid on

%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Discretizzazione del PI
%%%%%%%%%%%%%%%%%%%%%%%%%%%%
PI_tensione_bus_dc_bat_tf_disc=c2d(PI_tensione_bus_dc_bat_tf,Ts_dc_dc,'tustin');
PI_tensione_bus_dc_bat_tf_disc_num=PI_tensione_bus_dc_bat_tf_disc.Numerator{1};
PI_tensione_bus_dc_bat_tf_disc_den=-PI_tensione_bus_dc_bat_tf_disc.Denominator{1};

disp(' ');
disp('Regolatore tensione del bus dc discreto')
disp(['Rif(k) = ' num2str(PI_tensione_bus_dc_bat_tf_disc_den(2)) ' Rif(k-1) + ' num2str(PI_tensione_bus_dc_bat_tf_disc_num(1)) ' Err(k) + '...
    num2str(PI_tensione_bus_dc_bat_tf_disc_num(2)) ' Err(k-1)']);

disp(' ');
disp(' ');

Parametri_Controllori=[Parametri_Controllori...
    PI_tensione_bus_dc_bat_tf_disc_den(2) PI_tensione_bus_dc_bat_tf_disc_num(1) PI_tensione_bus_dc_bat_tf_disc_num(2)];

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                        %
%                Controllo dell'ampiezza di corrente                     %
%                       nella bobina ricevente                           %
%                                                                        %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Controllo dell'ampiezza della corrente nella bobina ricevente  
% mediante la tensione applicata alla bobina trasmittente.
% Il controllore è implementato nella sezione trasmittente perchè agisce sulla tensione
% generata su questa sezione. Il convertitore dc-ac lato trasmittente è
% attivo mentre il convertitore dc-ac lato ricevente agisce come raddrizzatore.
% Il riferimento di corrente è generato nella sezione ricevente perchè
% serve a regolare la potenza iniettata nel condensatore del bus dc lato ricevente.
% Il controllore riceve tramite radio l'errore di ampiezza della corrente nella bobina 
% ricevente e calcola il riferimento per la tensione da applicare alla bobina trasmittente.


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Specifiche per l'anello di controllo
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

banda_passante_corrente_bobina_rx_Hz=50;
margine_fase_corrente_bobina_rx_deg=85;

banda_passante_corrente_bobina_rx=banda_passante_corrente_bobina_rx_Hz*2*pi;
margine_fase_corrente_bobina_rx=margine_fase_corrente_bobina_rx_deg*pi/180;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Sistema da controllare
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% L'impianto da regolare è costituito dalle due bobine accoppiate in
% risonanza e la grandezza regolata è l'ampiezza della corrente della bobina ricevente.
% La funzione di trasferimento semplificata dell'impianto è data da
% FdT=1/(wM) come mostrato a pag. 8 dell'articolo. Nel cammino di
% retroazione c'è un filtro passa basso per l'estrazione del valore medio
% della corrente raddrizzata, proporzionale alla sua ampiezza.

% Funzione di trasferimento tra tensione sulla bobina tx e la corrente nella bobina rx
bobine_tf=tf(1/(w_dc_ac_nom*M_coil),1);

% Ritardo di trasmissione e di controllo
ritardo_trasm_contr_tf=series(ritardo_di_campionamento_dc_ac_tf,ritardo_di_trasmissione_tf);

% Funzione di trasferimento catena diretta dell'impianto
catena_diretta_bobine_ritardo_tf=series(bobine_tf,ritardo_trasm_contr_tf);

% Funzione di trasferimento a catena aperta 
impianto_controllo_Ibobina_rx_tf=series(catena_diretta_bobine_ritardo_tf,filtro_pb_coil_tf);

% Dall'analisi dei primi risultati si nota che con un controllore di tipo
% PI le frequenze elevate non sono attenuate efficacemente. Risulta quindi
% necessario aggiungere un ulteriore polo a frequenza superiore alla banda del
% sistema

freq_polo_aggiuntivo_corrente_bobina_rx_Hz=0.35e3;
w_polo_aggiuntivo_corrente_bobina_rx_rad_s=freq_polo_aggiuntivo_corrente_bobina_rx_Hz*2*pi;
tau_polo_aggiuntivo_corrente_bobina_rx_s=1/w_polo_aggiuntivo_corrente_bobina_rx_rad_s;

polo_aggiuntivo_corrente_bobina_rx_tf=tf(1,[tau_polo_aggiuntivo_corrente_bobina_rx_s 1]);

% Nel progettare il PI il polo aggiuntivo è considerato parte della catena diretta dell'impianto
catena_diretta_impianto_controllo_Ibobina_rx_polo_tf=series(catena_diretta_bobine_ritardo_tf,polo_aggiuntivo_corrente_bobina_rx_tf);

% Funzione di trasferimento complessiva dell'impianto a catena aperta con il polo. In retroazione c'è il filtro passa basso
impianto_controllo_Ibobina_rx_polo_tf=series(catena_diretta_impianto_controllo_Ibobina_rx_polo_tf,filtro_pb_coil_tf);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Progettazione del controllore
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Dall'analisi dei primi risultati si nota che con un controllore di tipo
% puramente integrale è sufficiente per ottenere le prestazioni desiderate


I_corrente_bobina_rx_k_1_tf=tf(1,[1 0]); % Solo integratore

catena_aperta_anello_corrente_bobina_rx_k_1_tf=series(I_corrente_bobina_rx_k_1_tf,impianto_controllo_Ibobina_rx_polo_tf);
[modulo_catena_aperta_corrente_bobina_rx,~,~]=bode(catena_aperta_anello_corrente_bobina_rx_k_1_tf,banda_passante_corrente_bobina_rx);

Ki_I_corrente_bobina_rx=1/modulo_catena_aperta_corrente_bobina_rx;

disp('Controllore ampiezza corrente bobina ricevente')
disp(' ');
disp(['Frequenza polo aggiuntivo = ' num2str(freq_polo_aggiuntivo_corrente_bobina_rx_Hz)]);
disp(['Ki anello ampiezza corrente bobina rx = ' num2str(Ki_I_corrente_bobina_rx)]);


% Funzione di traferimento PI
I_corrente_bobina_rx_polo_tf=Ki_I_corrente_bobina_rx*I_corrente_bobina_rx_k_1_tf;
% Funzione di trasferimento del controllore. Il controllore è formato dall' I e dal polo aggiuntivo
contr_corrente_bobina_rx_tf=series(I_corrente_bobina_rx_polo_tf,polo_aggiuntivo_corrente_bobina_rx_tf);

% Funzione di trasferimento ad anello aperto
catena_aperta_anello_corrente_Icoil_rx_tf=series(contr_corrente_bobina_rx_tf,impianto_controllo_Ibobina_rx_tf);

% Funzione di trasferimento ad anello chiuso. Il filtro passa basso è in retroazione
catena_chiusa_corrente_Icoil_rx_tf=feedback(series(catena_diretta_bobine_ritardo_tf,contr_corrente_bobina_rx_tf),filtro_pb_coil_tf,-1);


% figure(1)
% bode(impianto_controllo_Ibobina_rx_tf);
% hold on
% bode(impianto_controllo_Ibobina_rx_tf,banda_passante_corrente_bobina_rx,'o');
% grid on
% hold off
% 
% figure(2)
% bode(catena_aperta_anello_corrente_Icoil_rx_tf);
% hold on
% bode(catena_aperta_anello_corrente_Icoil_rx_tf,banda_passante_corrente_bobina_rx,'o');
% bode(catena_chiusa_corrente_Icoil_rx_tf);
% grid on
% hold off
% 
% figure(3)
% step(catena_chiusa_corrente_Icoil_rx_tf);
% grid on

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Discretizzazione del regolatore
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
contr_corrente_bobina_rx_tf=c2d(contr_corrente_bobina_rx_tf,Ts_dc_ac,'tustin');
contr_corrente_bobina_rx_tf_disc_num=contr_corrente_bobina_rx_tf.Numerator{1};
contr_corrente_bobina_rx_tf_disc_den=-contr_corrente_bobina_rx_tf.Denominator{1};

disp(' ');
disp('Controllore ampiezza corrente bobina ricevente discreto')
disp(['Rif(k) = ' num2str(contr_corrente_bobina_rx_tf_disc_den(2)) ' Rif(k-1) + ' num2str(contr_corrente_bobina_rx_tf_disc_den(3)) ' Rif(k-2) + '...
    num2str(contr_corrente_bobina_rx_tf_disc_num(1)) ' Err(k) + '...
    num2str(contr_corrente_bobina_rx_tf_disc_num(2)) ' Err(k-1) + ' num2str(contr_corrente_bobina_rx_tf_disc_num(3)) ' Err(k-2)']);

disp(' ');
disp(' ');


Parametri_Controllori=[Parametri_Controllori...
    contr_corrente_bobina_rx_tf_disc_den(2) contr_corrente_bobina_rx_tf_disc_den(3)...
    contr_corrente_bobina_rx_tf_disc_num(1) contr_corrente_bobina_rx_tf_disc_num(2) contr_corrente_bobina_rx_tf_disc_num(3)];


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                        %
%        Regolazione della tensione del bus dc sul lato ricevente        %
%    mediante scambio di potenza con la batteria del lato trasmittente   %
%  Condensatore sullo stesso lato della bobina con corrente controllata  %
%                                                                        %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Regolazione  della tensione del condensatore sul lato ricevente durante
% la carica della batteria sullo stesso lato.
% In questa condizione il condensatore sul lato ricevente viene simultaneamente
% caricato mediante la corrente nella bobina ricevente e scaricato mediante
% la corrente iniettata nella batteria.
% La tensione sul condensatore viene quindi regolata agendo sulla ampiezza della corrente 
% che circola nella bobina del lato ricevente.
% Questa corrente è controllato agendo sulla tensione applicata alla bobina
% trasmittente secondo l'anello di controllo descritto nel paragrafo precedente.
% L'anello di controllo della tensione sul lato ricevente è implementato nel
% lato ricevente ed è un anello di controllo esterno a quello che controlla la
% corrente nella bobina ricevente.
% Il regolatore di tensione è progettato supponendo di manipolare la potenza iniettata
% nel condensatore. Essa corrisponde alla potenza trasferita dal lato trasmittente a
% quello ricevente (a parte le perdite).

% Nella catena diretta dell'anello si considera il ritardo di campionamento. 
% Esiste un ulteriore ritardo di trasmissione dovuto alla trasmissione dell'errore
% di corrente alla sezione trasmittente. Esso è già considerato nella funzione di
% trasferimento dell'anello interno di controllo della corrente.
% Il regolatore genera un riferimento di potenza ma la grandezza controllata
% è una corrente. Si passa dall'una all'altra mediante una relazione di
% proporzionalità che suppone la tensione del bus dc del lato ricevente
% costante e pari al valore nominale.

% % % Sul lato trasmittente è generato un altro riferimento per la potenza
% % % trasmessa dal lato trasmittente al ricevente. Esso è calcolato per scaricare 
% % % il condensatore sul lato trasmittente.(non ancora implementato). 
% % % Questo secondo riferimento di potenza è inviato via radio al lato ricevente. 
% % % Qui viene confrontato con il riferimento della potenza necessaria a
% % % caricare il condensatore sul lato ricevente calcolato dal regolatore che viene qui 
% % % descritto.
% % % La potenza che viene effettivamente trasferita è la minore tra le due in modo da
% % % evitare di caricare o di scaricare eccessivamente i condensatori.
% % % Dal riferimento di potenza selezionato viene ricavato il corrispondente
% % % riferimento di ampiezza per la corrente della bobina ricevente. l'errore tra il
% % % riferimento di ampiezza e il valore di retroazione della ampiezza di corrente
% % % è spedito via radio alla sezione trasmittente del sistema e vengono processati 
% % % dall'anello di corrente qui implementato (vedi paragrafo precedente) che agisce
% % % sulla tensione generata sul lato trasmittente per controllare la corrente sul lato ricevente.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Specifiche per l'anello di controllo
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

banda_passante_tensione_Vdc_rx_Ptrasf_Hz=3;
margine_fase_tensione_Vdc_rx_Ptrasf_deg=80;

banda_passante_tensione_Vdc_rx_Ptrasf=banda_passante_tensione_Vdc_rx_Ptrasf_Hz*2*pi;
margine_fase_tensione_Vdc_rx_Ptrasf=margine_fase_tensione_Vdc_rx_Ptrasf_deg*pi/180;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Sistema da regolare
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% La variabile controllata è la tensione al quadrato.
% L'impianto da regolare, su cui è dimensionato il regolatore, è costituito
% dalla FdT dell'anello di corrente, da 2/(s Cdc), dal ritardo di campionamento 
% e dalla funzione di trasferimento dell'anello di corrente descritto nel 
% paragrafo precedente.

% Ritardo di campionamento
%ritardo_di_campionamento_dc_ac_tf;

% Funzione di trasferimento dell'anello di controllo della corrente
% nella bobina ricevente
anello_di_corrente_tf=catena_chiusa_corrente_Icoil_rx_tf;

% Legame tra riferimento di potenza e riferimento di corrente
Ptrasf_ref_tx_to_Icoil_ref_rx=pi/2*1/V_dc_0_rx;

% Legame tra corrente effettiva e potenza effettiva. E' il reciproco del
% coefficiente tra il riferimento di potenza e il riferimento di corrente
% Nel moltiplicarli lungo l'anello di controllo si elidono a vicenda
Icoil_rx_to_Ptrasf_rx=2/pi*V_dc_nom;

% Funzione di trasferimento 2/(sC)
due_su_sC_tf=tf(2,[C_bus_dc_dc 0]);

% Funzione di trasferimento complessiva a catena aperta del sistema di
% regolazione della tensione del bus dc lato ricevente mediante ampiezza
% della corrente nella bobina ricevente.
% Non compaiono i coefficienti Ptrasf_ref_rx_to_Icoil_ref_rx e Icoil_rx_to_Ptrasf_rx
% perchè il loro prodotto vale 1
impianto_regolazione_Vdc_rx_Ptrasf_tf=series(series(ritardo_di_campionamento_dc_ac_tf,anello_di_corrente_tf),due_su_sC_tf);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Progettazione del regolatore 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

[~,fase_tensione_Vdc_rx_Ptrasf_deg,~]=bode(impianto_regolazione_Vdc_rx_Ptrasf_tf,banda_passante_tensione_Vdc_rx_Ptrasf);
fase_tensione_Vdc_rx_Ptrasf=fase_tensione_Vdc_rx_Ptrasf_deg*pi/180;

tau_reg_tensione_Vdc_rx_Ptrasf=1/banda_passante_tensione_Vdc_rx_Ptrasf*tan(-pi/2+margine_fase_tensione_Vdc_rx_Ptrasf-fase_tensione_Vdc_rx_Ptrasf); % Costante di tempo del regolatore di tensione

reg_tensione_Vdc_rx_Ptrasf_k_1_tf=tf([tau_reg_tensione_Vdc_rx_Ptrasf 1],[tau_reg_tensione_Vdc_rx_Ptrasf 0]); % Funzione di traferimento PI con guadagno proporzionale unitario

catena_aperta_anello_tensione_Vdc_rx_Ptrasf_tf_k_1=series(reg_tensione_Vdc_rx_Ptrasf_k_1_tf,impianto_regolazione_Vdc_rx_Ptrasf_tf);
[modulo_catena_aperta_tensione_Vdc_rx_Ptrasf,~,~]=bode(catena_aperta_anello_tensione_Vdc_rx_Ptrasf_tf_k_1,banda_passante_tensione_Vdc_rx_Ptrasf);
Kp_PI_tensione_Vdc_rx_Ptrasf=1/modulo_catena_aperta_tensione_Vdc_rx_Ptrasf;
Ki_PI_tensione_Vdc_rx_Ptrasf=Kp_PI_tensione_Vdc_rx_Ptrasf/tau_reg_tensione_Vdc_rx_Ptrasf;

PI_tensione_Vdc_rx_Ptrasf_tf=tf(Kp_PI_tensione_Vdc_rx_Ptrasf*[tau_reg_tensione_Vdc_rx_Ptrasf 1],[tau_reg_tensione_Vdc_rx_Ptrasf 0]); % Funzione di traferimento PI

% Funzione di trasferimento ad anello aperto
catena_aperta_tensione_Vdc_rx_Ptrasf_tf=series(PI_tensione_Vdc_rx_Ptrasf_tf,impianto_regolazione_Vdc_rx_Ptrasf_tf);

% Funzione di trasferimento ad anello chiuso
catena_chiusa_tensione_Vdc_rx_Ptrasf_tf=feedback(catena_aperta_tensione_Vdc_rx_Ptrasf_tf,1,-1);


disp('Regolatore tensione del bus dc sul lato ricevente con scambio di potenza con il lato trasmittente')
disp(' ');
disp(['Kp anello di tensione bus dc rx = ' num2str(Kp_PI_tensione_Vdc_rx_Ptrasf)]);
disp(['Ki anello di tensione bus dc rx = ' num2str(Ki_PI_tensione_Vdc_rx_Ptrasf)]);


% figure(1)
% bode(impianto_regolazione_Vdc_rx_Ptrasf_tf);
% hold on
% bode(impianto_regolazione_Vdc_rx_Ptrasf_tf,banda_passante_tensione_Vdc_rx_Ptrasf,'o');
% grid on
% hold off
% 
% figure(2)
% bode(catena_aperta_tensione_Vdc_rx_Ptrasf_tf);
% hold on
% bode(catena_aperta_tensione_Vdc_rx_Ptrasf_tf,banda_passante_tensione_Vdc_rx_Ptrasf,'o');
% bode(catena_chiusa_tensione_Vdc_rx_Ptrasf_tf);
% grid on
% hold off
% 
% figure(3)
% step(catena_chiusa_tensione_Vdc_rx_Ptrasf_tf);
% grid on

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Discretizzazione del regolatore
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc=c2d(PI_tensione_Vdc_rx_Ptrasf_tf,Ts_dc_ac,'tustin');
reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc_num=reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc.Numerator{1};
reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc_den=-reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc.Denominator{1};

disp(' ');
disp('Regolatore tensione del bus dc lato ricevente discreto')
disp(['Rif(k) = ' num2str(reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc_den(2)) ' Rif(k-1) + ' num2str(reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc_num(1)) ' Err(k) + '...
    num2str(reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc_num(2)) ' Err(k-1)']);

disp(' ');
disp(' ');

Parametri_Controllori=[Parametri_Controllori...
    reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc_den(2) reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc_num(1) reg_tensione_locale_Vdc_rx_Ptrasf_tf_disc_num(2)];


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                        %
%         Regolazione della tensione del bus dc lato trasmittente        %
%      mediante scambio di potenza con la batteria del lato ricevente    %
%  Condensatore sul lato opposto della bobina con corrente controllata   %
%                                                                        %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Regolazione  della tensione del condensatore sul lato trasmittente durante
% la carica della batteria del lato ricevente.
% In questa condizione il condensatore sul lato trasmittente viene simultaneamente
% caricato mediante la corrente prelevata dalla batteria e scaricato mediante
% la potenza trasferita al lato ricevente.
% La tensione sul condensatore viene regolata agendo sulla ampiezza della corrente 
% che circola nella bobina del lato ricevente.
% Il regolatore di tensione è progettato supponendo di manipolare la potenza iniettata
% nel condensatore. Essa corrisponde alla potenza trasferita dal lato trasmittente a
% quello ricevente (a parte le perdite).
% Il riferimento di potenza è inviato via radio al lato ricevente. 
% Qui viene confrontato con il riferimento della potenza necessaria a
% caricare il condensatore sul lato ricevente calcolato dal regolatore 
% descritto nel paragrafo precedente.
% La potenza che viene effettivamente trasferita è la minore tra le due in modo da
% evitare di caricare o di scaricare eccessivamente i condensatori.
% Nella catena diretta dell'anello si considera il ritardo di campionamento e
% il ritardo di trasmissione.
% Esiste un ulteriore ritardo di trasmissione dovuto alla trasmissione dell'errore
% di corrente alla sezione trasmittente. Esso è già considerato nella funzione di
% trasferimento dell'anello interno di controllo della corrente.
% Il regolatore genera un riferimento di potenza ma la grandezza controllata
% è una corrente. Si passa dall'una all'altra mediante una relazione di
% proporzionalità che suppone la tensione del bus dc del lato ricevente
% costante e pari al valore nominale.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Specifiche per l'anello di controllo
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

banda_passante_tensione_Vdc_tx_Ptrasf_Hz=3;
margine_fase_tensione_Vdc_tx_Ptrasf_deg=80;

banda_passante_tensione_Vdc_tx_Ptrasf=banda_passante_tensione_Vdc_tx_Ptrasf_Hz*2*pi;
margine_fase_tensione_Vdc_tx_Ptrasf=margine_fase_tensione_Vdc_tx_Ptrasf_deg*pi/180;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Sistema da regolare
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% La variabile controllata è la tensione al quadrato.
% L'impianto da regolare, su cui è dimensionato il regolatore, è costituito
% dal ritardo di campionamento, dal ritardo di trasmissione, dalla funzione
% di trasferimento dell'anello di controllo della corrente nella bobina ricevente,
% dalla conversione tra corrente e potenza trasferita, dalla conversione tra potenza
% trasferita e corrente nella bobina trasmittente e dal condensatore.

% Ritardo di campionamento
% ritardo_di_campionamento_dc_ac_tf;

% Funzione di trasferimento dell'anello di controllo della corrente
% nella bobina ricevente
anello_di_corrente_tf=catena_chiusa_corrente_Icoil_rx_tf;

% Legame tra riferimento di potenza calcolato sul lato trasmittente e
% riferimento di corrente calcolato sul lato ricevente.
% N.B. Il regolatore è sul lato trasmittente e calcola un riferimento
% di potenza trasferita al lato ricevente. Si suppone che questa potenza 
% raggiunga effettivamente il lato ricevente
Ptrasf_ref_tx_to_Icoil_ref_rx=pi/2*1/V_dc_nom;

% Legame tra corrente effettiva e potenza effettiva. E' il reciproco del
% coefficiente tra il riferimento di potenza e il riferimento di corrente
% Nel moltiplicarli lungo l'anello di controllo si elidono a vicenda
Icoil_rx_to_Ptrasf_rx=2/pi*V_dc_nom;

% Funzione di trasferimento 2/(sC)
due_su_sC_tf=tf(2,[C_bus_dc_dc 0]);

% Funzione di trasferimento complessiva a catena aperta del sistema di
% regolazione della tensione del bus dc lato trasmittente mediante ampiezza
% della corrente nella bobina ricevente.
% Non compaiono i coefficienti Ptrasf_ref_rx_to_Icoil_ref_rx e Icoil_rx_to_Ptrasf_rx
% perchè il loro prodotto vale 1.
%ritardo_campionamento_trasmissione_tf=series(ritardo_di_campionamento_dc_ac_tf,ritardo_di_trasmissione_tf);
ritardo_campionamento_trasmissione_tf=ritardo_di_trasmissione_tf; % Non c'è il ritardo di campionamento perchè è considerato nell'anello di corrente
impianto_regolazione_Vdc_tx_Ptrasf_tf=series(series(ritardo_campionamento_trasmissione_tf,anello_di_corrente_tf),due_su_sC_tf);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Progettazione del regolatore 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

[~,fase_tensione_Vdc_tx_Ptrasf_deg,~]=bode(impianto_regolazione_Vdc_tx_Ptrasf_tf,banda_passante_tensione_Vdc_tx_Ptrasf);
fase_tensione_Vdc_tx_Ptrasf=fase_tensione_Vdc_tx_Ptrasf_deg*pi/180;

tau_reg_tensione_Vdc_tx_Ptrasf=1/banda_passante_tensione_Vdc_tx_Ptrasf*tan(-pi/2+margine_fase_tensione_Vdc_tx_Ptrasf-fase_tensione_Vdc_tx_Ptrasf); % Costante di tempo del regolatore di tensione

PI_tensione_Vdc_tx_Ptrasf_k_1_tf=tf([tau_reg_tensione_Vdc_tx_Ptrasf 1],[tau_reg_tensione_Vdc_tx_Ptrasf 0]); % Funzione di traferimento PI con guadagno proporzionale unitario

catena_aperta_anello_tensione_Vdc_tx_Ptrasf_tf_k_1=series(PI_tensione_Vdc_tx_Ptrasf_k_1_tf,impianto_regolazione_Vdc_tx_Ptrasf_tf);
[modulo_catena_aperta_tensione_Vdc_tx_Ptrasf,~,~]=bode(catena_aperta_anello_tensione_Vdc_tx_Ptrasf_tf_k_1,banda_passante_tensione_Vdc_tx_Ptrasf);
Kp_PI_tensione_Vdc_tx_Ptrasf=1/modulo_catena_aperta_tensione_Vdc_tx_Ptrasf;
Ki_PI_tensione_Vdc_tx_Ptrasf=Kp_PI_tensione_Vdc_tx_Ptrasf/tau_reg_tensione_Vdc_tx_Ptrasf;

disp('Regolatore tensione del bus dc sul lato trasmittente con scambio di potenza con il lato ricevente')
disp(' ');
disp(['Kp anello di tensione bus dc tx = ' num2str(Kp_PI_tensione_Vdc_tx_Ptrasf)]);
disp(['Ki anello di tensione bus dc tx = ' num2str(Ki_PI_tensione_Vdc_tx_Ptrasf)]);


PI_tensione_remoto_Vdc_tx_Ptrasf_tf=Kp_PI_tensione_Vdc_tx_Ptrasf*PI_tensione_Vdc_tx_Ptrasf_k_1_tf;


% Funzione di trasferimento ad anello aperto
catena_aperta_tensione_Vdc_tx_Ptrasf_tf=series(PI_tensione_remoto_Vdc_tx_Ptrasf_tf,impianto_regolazione_Vdc_tx_Ptrasf_tf);

% Funzione di trasferimento ad anello chiuso
catena_chiusa_tensione_Vdc_tx_Ptrasf_tf=feedback(catena_aperta_tensione_Vdc_tx_Ptrasf_tf,1,-1);

 
% figure(1)
% bode(impianto_regolazione_Vdc_tx_Ptrasf_tf);
% hold on
% bode(impianto_regolazione_Vdc_tx_Ptrasf_tf,banda_passante_tensione_Vdc_tx_Ptrasf,'o');
% grid on
% hold off
% 
% figure(2)
% bode(catena_aperta_tensione_Vdc_tx_Ptrasf_tf);
% hold on
% bode(catena_aperta_tensione_Vdc_tx_Ptrasf_tf,banda_passante_tensione_Vdc_tx_Ptrasf,'o');
% bode(catena_chiusa_tensione_Vdc_tx_Ptrasf_tf);
% grid on
% hold off
% 
% figure(3)
% step(catena_chiusa_tensione_Vdc_tx_Ptrasf_tf);
% grid on

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Discretizzazione del regolatore
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc=c2d(PI_tensione_remoto_Vdc_tx_Ptrasf_tf,Ts_dc_ac,'tustin');
PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc_num=PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc.Numerator{1};
PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc_den=-PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc.Denominator{1};

disp(' ');
disp('Regolatore tensione del bus dc lato trasmittente discreto')
disp(['Rif(k) = ' num2str(PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc_den(2)) ' Rif(k-1) + ' num2str(PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc_num(1)) ' Err(k) + '...
    num2str(PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc_num(2)) ' Err(k-1)']);

disp(' ');
disp(' ');

Parametri_Controllori=[Parametri_Controllori...
    PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc_den(2) PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc_num(1) PI_tensione_remoto_Vdc_tx_Ptrasf_tf_disc_num(2)];


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                        %
%                Limitazione dell'ampiezza di corrente                   %
%                      nella bobina trasmittente                         %
%                                                                        %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Progettazione dell'algoritmo di controllo del convertitore dc-ac 
% La grandezza controllata è la corrente che fluisce nella bobina trasmittente.
% Questo anello di controllo serve a limitare la corrente nel caso si abbia
% un accoppiamento insufficiente. Lavora insieme all'anello di controllo
% della corrente nella bobina ricevente e ne limita il riferimento di
% tensione. Questo anello di controllo è attivo nella sezione trasmittente del
% sistema WPT.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Specifiche per l'anello di controllo
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
banda_passante_corrente_trasm_Hz=500;
margine_fase_corrente_trasm_deg=80;

banda_passante_corrente_trasm=banda_passante_corrente_trasm_Hz*2*pi;
margine_fase_corrente_trasm=margine_fase_corrente_trasm_deg*pi/180;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Sistema da controllare
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Il sistema da controllare è costituito dal convertitore dc-ac, dalla bobina
% e dalla sua resistenza serie. Nel cammino di retroazione c'è il filtro pb
% che calcola l'ampiezza della corrente. 
% Si suppone che la bobina non sia accoppiata perchè quest è la condizione
% peggiore dal punto di vista della limitazione dela corrente.

Req_L_coil=(R_coil+2*Rds_on_dc_ac); % Resistenza equivalente in serie all'induttanza di filtro.
                                    % Tiene conto anche della resistenza di conduzione degli interruttori

% Funzione di trasferimento dalla ampiezza di tensione ai capi della bobina
% ampiezza di corrente nella bobina. Ricavata numericamente dalle simulazioni
Coil_trasm=tf(1/Req_L_coil,[0.4e-3 1]);

% Catena retroazione = filtro pb + campionamento
Retroazione_corrente_trasm_tf=series(filtro_pb_coil_tf,ritardo_di_campionamento_dc_dc_tf);
% Funzione trasferimento a catena aperta bobina + filtro pb per
% rilevamento dell'ampiezza della corrente + campionamento
Coil_filtro_pb_campionamento_tf=series(series(Coil_trasm,filtro_pb_coil_tf),ritardo_di_campionamento_dc_dc_tf);

[~,fase_coil_filtro_pb_camp_deg,~]=bode(Coil_filtro_pb_campionamento_tf,banda_passante_corrente_trasm);
fase_coil_filtro_pb_camp=fase_coil_filtro_pb_camp_deg*pi/180;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   Progettazione del controllore PI
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Determinazione della costante di tempo del controllore PI
tau_PI_corrente_trasm=1/banda_passante_corrente_trasm*tan(-pi/2+margine_fase_corrente_trasm-fase_coil_filtro_pb_camp); % Costante di tempo del PI di corrente

PI_corrente_trasm_k_1_tf=tf([tau_PI_corrente_trasm 1],[tau_PI_corrente_trasm 0]); % Funzione di traferimento PI con guadagno proporzionale unitario

catena_aperta_anello_corrente_trasm_k_1_tf=series(PI_corrente_trasm_k_1_tf,Coil_filtro_pb_campionamento_tf);
% Guadagno a catena aperta alla frequenza di taglio con controllore controllore avente guadagno proporzionale unitario
[modulo_catena_aperta_corrente_trasm,~,~]=bode(catena_aperta_anello_corrente_trasm_k_1_tf,banda_passante_corrente_trasm); 
Kp_PI_corrente_trasm=1/modulo_catena_aperta_corrente_trasm;       % Guadagno proporzionale del controllore. Porta a 1 il guadagno a catena aperta.
Ki_PI_corrente_trasm=Kp_PI_corrente_trasm/tau_PI_corrente_trasm;  % Guadagno integrale del controllore.
disp(' ');
disp('Controllo ampiezza corrente bobina trasmittente')
disp(' ');
disp(['Kp anello di ampiezza corrente bobina trasmittente = ' num2str(Kp_PI_corrente_trasm)]);
disp(['Ki anello di ampiezza corrente bobina trasmittente = ' num2str(Ki_PI_corrente_trasm)]);
disp(' ');

PI_corrente_trasm_tf=tf(Kp_PI_corrente_trasm*[tau_PI_corrente_trasm 1],[tau_PI_corrente_trasm 0]); % Funzione di traferimento PI

% Funzione di trasferimento ad anello aperto del sistema con regolatore
catena_aperta_anello_corrente_trasm_tf=series(PI_corrente_trasm_tf,Coil_filtro_pb_campionamento_tf);

% Funzione di trasferimento ad anello chiuso del sistema con regolatore
catena_diretta_anello_corrente_trasm_tf=series(PI_corrente_trasm_tf,Coil_trasm);
catena_chiusa_anello_corrente_trasm_tf=feedback(catena_diretta_anello_corrente_trasm_tf,Retroazione_corrente_trasm_tf,-1);

% figure(1)
% bode(Coil_filtro_pb_campionamento_tf);
% hold on
% bode(Coil_filtro_pb_campionamento_tf,banda_passante_corrente_trasm,'o');
% grid on
% hold off
% 
% figure(2)
% bode(catena_aperta_anello_corrente_trasm_tf);
% hold on
% bode(catena_aperta_anello_corrente_trasm_tf,banda_passante_corrente_trasm,'o');
% bode(catena_chiusa_anello_corrente_trasm_tf);
% grid on
% hold off
% 
% figure(3)
% step(catena_chiusa_anello_corrente_trasm_tf);
% grid on

%%%%%%%%%%%%%%%%%%%%%%%%%%
% Discretizzazione del PI
%%%%%%%%%%%%%%%%%%%%%%%%%%
disp('Controllore discreto ampiezza corrente bobina trasmittente')
PI_corrente_trasm_tf_disc=c2d(PI_corrente_trasm_tf,Ts_dc_dc,'tustin');
PI_corrente_trasm_tf_disc_num=PI_corrente_trasm_tf_disc.Numerator{1};
PI_corrente_trasm_tf_disc_den=-PI_corrente_trasm_tf_disc.Denominator{1};
disp(['Rif(k) = ' num2str(PI_corrente_trasm_tf_disc_den(2)) ' Rif(k-1) + ' num2str(PI_corrente_trasm_tf_disc_num(1)) ' Err(k) + '...
    num2str(PI_corrente_trasm_tf_disc_num(2)) ' Err(k-1)']);
disp(' ');
disp(' ');

Parametri_Controllori=[Parametri_Controllori...
    PI_corrente_trasm_tf_disc_den(2) PI_corrente_trasm_tf_disc_num(1) PI_corrente_trasm_tf_disc_num(2)];



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                 %
%   Parametri simulink per la simulazione del sistema completo    %
%                                                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Selettore_Trasferimento_da_Tx_a_Rx_1_da_Rx_a_Tx_0=1; % 1 Il trasferimento di potenza avviene dal lato tx verso il lato rx
                                                     % 0 Il trasferimento di potenza avviene dal lato rx verso il lato tx

Annulla_Scambio_Potenza=2;                                                                                 


Selettore_Switching_0_Valori_Medi_1_dc_dc_tx=1;  % 1 La simulazione del convertitore dc-dc è effettuata usando il modello ai valori medi
                                                 % 0 La simulazione del convertitore dc-dc è effettuata usando il modello circuitale.
                                                 % E' più accurata ma molto più lenta
Selettore_Switching_0_Valori_Medi_1_dc_dc_rx=1; 

Selettore_No_Dead_Time_0_Dead_Time_1_tx=0;   % 1 Se è abilitato il modello circuitale del convertitore dc-dc simula anchei tempi morti
                                             % 0 I tempi morti non sono simulati
Selettore_No_Dead_Time_0_Dead_Time_1_rx=0;

Selettore_bat_Batteria_0_Condensatore_1_tx=1; % 1 Rappresenta la batteria mediante un condensatore in modo da evidenziare la crica e la scarica
                                              % 0 Rappresenta la batteria mediante un generatore di tensione. Si può controllare la corrente, ma
                                              % la tensione rimane costante
Selettore_bat_Batteria_0_Condensatore_1_rx=1;

% Imposta il passo d integrazione della simulazione
if((Selettore_Switching_0_Valori_Medi_1_dc_dc_tx==0)||(Selettore_Switching_0_Valori_Medi_1_dc_dc_rx==0))
    Passo_integrazione_massimo=T_dc_ac/250;
else
    Passo_integrazione_massimo=T_dc_ac/100;
end

Durata_Simulazione=6; % Durata della simulazione in secondi

controllo_Completo_v14;  % File Simulink
%set_param(bdroot,'SFSimEnableDebug','on');
set_param(bdroot,'SFSimEnableDebug','off');


