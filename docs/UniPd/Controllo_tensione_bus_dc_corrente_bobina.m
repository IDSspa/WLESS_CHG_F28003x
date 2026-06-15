function [I_coil_loc_err, duty_cycle_PWM_A, duty_cycle_PWM_B, abilita_PWM, duty_cycle_PS_A, duty_cycle_PS_B, abilita_PS]  =...
                            Controllo_Tensione_Bus_dc(I_coil_loc_rif, I_coil_loc, I_coil_rem_err, V_dc_Pbat_rif, V_dc, V_bat, I_L_A, I_L_B, I_bat_rif_max, I_bat_rif_min, Tx_1_Rx_0)

% Questa funzione implementa:
% 1) Il controllo della corrente scambiata tra la batteria e il condensatore 
% del bus dc tramite il convertitore dc-dc.
% 2) La regolazione della tensione del bus dc utilizzando l'anello di corrente 
% implementato al punto 1)
% 3) Il controllo della corrente nella bobina remota (quella dell'altra sezione del sistema)
% mediante la manipolazione della tensione generata dal convertitore dc-ac
% locale (di questa sezione del convertitore). Questo anello di corrente
% sarà sfruttato come anello interno da un anello di regolazione della
% tensione del condensatore che ancora non è implementato nella funzione.

% Nella realizzazione in C questo codice è parte di una routine di servizio
% di un interrupt per cui non ci saranno varibili di ingreso e uscita e la
% funzione dovrà operare su variabili condivise con le altre sezioni del
% firmware.
% Tutte le grandezze sono supposte di tipo float e sono scalate nelle unità
% naturali (Volt, Ampere,...).
% Nel protipo realizzato a Padova la routine dei serviio veniva eseguita
% come risposta ad un interrup generato alla fine della conversione
% analogico-digitale delle varie grandezze di interesse. Di conseguenza
% quando la routine viene eseguita tutti i risultati della conversione sono
% già disponibili. A sua volta, la conversione analogico-digitale è
% effettuata all'inizio del periodo di PWM.

% Al fine di garantire un tempo di eleborazione sufficiente alla routine di
% servizio si è supposto che essa venga eseguita con una frequenza pari a
% 1/4 di quella di alimentazione delle bobine. (85 kHz/4 = 21.25 kHz).
% Quindi il periodo di campionamento risulta di circa 47 us.
% La medesima routine di servizion deve gestire sia il convertitore dc-dc
% che il convertitore dc-ac. La frequenza di switching del primo era stata
% fissata a 120 kHz come nel prototipo Texas. questa frequenza non è multipla
% di quella di campionamento per cui si propone di alzare la frequenza di
% switching del convertitore dc-dc a 6 x 21.25 kHz = 127.5 kHz. In questo
% modo non sarà necessario ridimensionare le induttanze di filtro collegate
% all'ingresso del convertitore dc-dc.

% Variabili in ingresso
%
% I_coil_loc_rif, I_coil_loc sono il valore di riferimento e il valore attuale
% dell'ampiezza della corrente nella bobina locale. Il riferimento viene 
% generato da un anello di controllo della tensione ancora non implementato.
% Il valore attuale viene acquisito tramite i convertitori A/D a partire dal
% segnale generato dal circuito che trasduce l'ampiezza della corrente.
% La differenza
%
% I_coil_loc_err=I_coil_loc_rif-I_coil_loc
%
% viene inviata tramite radio all'altra sezione del sistema.
%
% I_coil_rem_err è l'errore di ampiezza di corrente nella bobina remota
% (dell'altra sezione del sistema). E' ricevuto via radio.
% In base a questo errore viene calcolata la tensione generata dal convertitore
% dc-ac locale per controllare l'ampiezza di corrente della bobina remota.
% 
% 
% V_dc_rif, V_dc, sono i valori di riferimento e il valore attuale della
% tensione del bus dc. Durante il funzionamento V_dc_rif è costante e assume
% due valori diversi a seconda che la batteria venga caricata o scaricata.
% 
% V_bat è il valore attuale della tensione di batteria. E'utilizzato per calcolare
% la potenza massima che può essere scambiata con la batteria e il valore dei
% duty cycle da applicare alle due gambe del convertitore dc-dc.
% 
% I_L_A, I_L_B, sono i valori attuali delle correnti nei due rami del
% convertitore dc-dc 
%
% I_bat_rif_max, I_bat_rif_min, sono il limite superiore (durante la scarica)
% e inferiore (durante la carica) della corrente che può essere estratta o
% iniettata nella batteria. Sono forniti dal BMS.
%
% Tx_1_Rx_0 specifica se la potenza deve essere trasmessa o ricevuta.
% Quando Tx_1_Rx_0=1 la potenza viene trasmessa e il convertiore dc-ac è
% attivato e comandato per controllare l'ampiezza di corrente nella bobina 
% remota. Quando Tx_1_Rx_0=0 la potenza viene ricevuta e il convertitore
% ac-dc viene disattivato e agisce come un raddrizzatori.
% In entrambi i casi il convetitore dc-dc viene controllato per scambiare
% potenza tra la batteria e il condensatore del bus dc.

% Variabili in uscita
%
% I_coil_loc_err è l'errore locale di corrente. Viene trasmesso via radio
% all'altra sezione del sistema dove diventa la variabile in ingresso
% I_coil_rem_err.
%
% duty_cycle_PWM_A, duty_cycle_B sono i duty cycle per i due rami del
% convertitore buck-boost dc-dc.  I duty cycle sono compresi tra 0 e 1.
% Vanno scalati opportunamente per essere impiegati nella periferica del
% microcontrollore che genera i comandi di PWM.
%
% abilita_PWM viene utilizzato per abilitare il convertitore dc-dc quando viene
% effettuato uno scambio di potenza tra le due sezioni del sistema.
% L'abilitazione viene leggermente ritardata rispetto all'accensione del sistema
% WPT per permettere ai vari filtri che processano i segnali analogici di
% raggiungere la condizione di regime.
%
% duty_cycle_PS_A, duty_cycle_PS_B, sono i duty cycle per i due rami del
% convertitore dc-ac.  I duty cycle sono compresi tra 0 e 1 e sono legati
% allo sfasamento tra le due tensioni ad onda quadra generate sui punti centrali
% delle gambe del convertitore ac-dc. Maggiori dettagli si trovano nella
% relazione "Controllo del convertitore dc-ac"
%
% abilita_PS viene utilizzato per abilitare il convertitore dc-ac quando deve
% essere trasmessa potenza all'altra sezione del sistema.
% L'eventuale abilitazione viene leggermente ritardata rispetto all'accensione
% del sistema WPT per permettere ai vari filtri che processano i segnali
% analogici di raggiungere la condizione di regime.


% Per ottenere i duty-cycle per il convertitore dc-ac è necessario
% calcolare la funzione
% 1/pi*arcoseno(V_ac_rif/V_ac_rif_max).
% A questo scopo è utilizzata una tabella per effettuare una interpolazione lineare.
%
% Tabella usata per l'interpolazione della funzione 1/pi*arcoseno() tra 0 e 1
% La tabella contiene 101 elementi. L'ultimo è usato solo per semplificare il codice.
% L'indice della tabella in C varia tra 0 e 100. Corrisponde all'argomento dell'arcoseno moltiplicato per 100. 
tabella_arcsin=[0.000000000000000,   0.003183151915873,   0.006366622213270,   0.009550729560432,   0.012735793199755,   0.015922133236660, ...
            0.019110070930640,   0.022299928989202,   0.025492031865477,   0.028686706060258,   0.031884280429260,   0.035085086496430,...
            0.038289458774147,   0.041497735091205,   0.044710256929508,   0.047927369770437,   0.051149423451922,   0.054376772537300,...
            0.057609776697097,   0.060848801104951,   0.064094216848975,   0.067346401359940,   0.070605738857752,   0.073872620817828,...
            0.077147446459050,   0.080430623255166,   0.083722567471605,   0.087023704729853,   0.090334470601733,   0.093655311236095,...
            0.096986684020678,   0.100329058282131,   0.103682916027458,   0.107048752730465,   0.110427078167105,   0.113818417304015,...
            0.117223311244961,   0.120642318240358,   0.124076014765585,   0.127524996674404,   0.130989880434455,   0.134471304452546,...
            0.137969930498342,   0.141486445235958,   0.145021561874111,   0.148576021946683,   0.152150597236966,   0.155746091860452,...
            0.159363344522883,   0.163003230972354,   0.166666666666667,   0.170354609679922,   0.174068063875524,   0.177808082376468,...
            0.181575771368100,   0.185372294273510,   0.189198876347599,   0.193056809742680,   0.196947459106530,   0.200872267783327,...
            0.204832764699133,   0.208830572026983,   0.212867413742587,   0.216945125200853,   0.221065663886429,   0.225231121519470,...
            0.229443737731699,   0.233705915569418,   0.238020239131091,   0.242389493710302,   0.246816688893365,   0.251305085159287,...
            0.255858224653840,   0.260479966967229,   0.265174530946820,   0.269946543837384,   0.274801099381575,   0.279743826961257,...
            0.284780974456357,   0.289919508299921,   0.295167235300867,   0.300532952314905,   0.306026631958643,   0.311659655572965,...
            0.317445109006172,   0.323398163238602,   0.329536571616801,   0.335881330554762,   0.342457574575956,   0.349295816015145,...
            0.356433706871294,   0.363918619603224,   0.371811566302050,   0.380193416581985,   0.389175313395541,   0.398917375895740,...
            0.409665529398267,   0.421834068069044,   0.436231439141480,   0.454946586355588,   0.500000000000000,   0.454946586355588];


% Costanti di controllo
% Possono essere delle variabili globali definite in un altro file oppure
% delle costanti definite in un file esterno tramite #define e poi
% importate tramite #include

% Sono dei limiti fittizi imposti alle tensioni in modo che l'agoritmo di
% controllo non si trovi a lavorare con valori troppo grandi o troppo
% piccoli. Nel sistema reale non dovrebbe essere necessario gestire queste
% limitazioni
V_bat_max_alg=218;
V_bat_min_alg=37.5;
V_dc_max_alg=500;
V_dc_min_alg=62.5;

% Parametri dei controllori
% Controllore della corrente nei due rami del convertitore dc-dc

% Banda Passante 250 Hz. fs=21.25 kHz (85 kHz/4)
K_I_L_err=0.533694299666651;    % Guadagno applicato all'errore di corrente attuale
K_I_L_err_p=-0.526383425589328; % Guadagno applicato all'errore di corrente relativo all'istante di campionamento precedente


% Controllore della tensione del bus dc
% Banda Passante 10 Hz, fs=21.25 kHz (85 kHz/4)
K_V_dc_2_Pbat_err=0.046777788128766;   % Guadagno applicato all'errore di tensione attuale
K_V_dc_2_Pbat_err_p=-0.046768069962416;    % Guadagno applicato all'errore di tensione precedente


% Controllore dell'ampiezza della corrente nella bobina remota 
% Banda Passante 85 Hz. Ts=1/21.25 kHz
K_I_coil_err=0.002436907237795;      % Guadagno applicato all'errore di corrente attuale
K_I_coil_err_p=0.004873814475590;    % Guadagno applicato all'errore di corrente di un passo precedente
K_I_coil_err_pp=0.002436907237795;   % Guadagno applicato all'errore di corrente di due passi precedenti

K_V_coil_rif_p=1.901603650787137;    % Guadagno applicato al riferimento di tensione del passo precedente
K_V_coil_rif_pp=-0.901603650787137;  % Guadagno applicato al riferimento di tensione di due passi precedenti


% Ritardo abilitazione PWM
Ritardo_PWM=100; % Misurato in periodi di campionamento

% Variabili relative all'istante di campionamento precedente.
% Devono essere mantenute tra una chiamata e l'altra della routine di
% servizio.
% Sono equivalenti alle varibili "static" in C
persistent Inizializzato
persistent Contatore
persistent I_L_err_A_p
persistent I_L_err_B_p % Errori di corrente nei due rami del convertitore buck-boost al passo di campionamento precedente
persistent V_L_rif_A_p
persistent V_L_rif_B_p % Riferimenti di tensione da applicare alle induttanza di filtro del buck-boost al passo precedente 
persistent V_dc_2_Pbat_err_p % Errore di tensione al passo precedente 
persistent P_bat_rif_p % Riferimento di potenza al passo precedente 
persistent I_coil_rem_err_p
persistent I_coil_rem_err_pp % Errore di corrente nella bobina all'istante di campionamento precedente
persistent V_ac_rif_p
persistent V_ac_rif_pp % Riferimenti di tensione da applicare alla bobina all'istante precedente

% Inizializzazione delle varibili persistent
% In C corrisponde ad assegnare il valore iniziale alla variabili statiche
% al momento della dichiarazione, per cui non serve una struttura "if"
if isempty(Inizializzato)
    Inizializzato = 1;
    Contatore = 0;
    I_L_err_A_p = 0;
    I_L_err_B_p = 0;
    V_L_rif_A_p = 0;
    V_L_rif_B_p = 0;
    V_dc_2_Pbat_err_p = 0;
    P_bat_rif_p = 0;
    I_coil_rem_err_p = 0;
    I_coil_rem_err_pp = 0;
    V_ac_rif_p = 0;
    V_ac_rif_pp = 0;
end

if(Contatore<Ritardo_PWM)
    Contatore=Contatore+1;
    abilita_PWM=0;
    abilita_PS=0;
else
    abilita_PWM=1;
    if(Tx_1_Rx_0==1)
        abilita_PS=1;
    else
        abilita_PS=0;
    end 
end   


% Limita i valori misurati di V_bat e V_dc per evitare problemi di calcolo
% (nell'applicazione normale non dovrebbe mai succedere)

if(V_bat>V_bat_max_alg)
    V_bat=V_bat_max_alg;
end    

if(V_bat<V_bat_min_alg)
    V_bat=V_bat_min_alg;
end

if(V_dc>V_dc_max_alg)
    V_dc=V_dc_max_alg;
end    

if(V_dc<V_dc_min_alg)
    V_dc=V_dc_min_alg;
end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%  Controllo della tensione del bus dc mediante scambio di potenza con la batteria   % 
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


% Limiti di potenza trasferibile
P_bat_rif_max=V_bat * I_bat_rif_max;
P_bat_rif_min=V_bat * I_bat_rif_min;

% Errore tra riferimento al quadrato di tensione e tensione attuale al quadrato
V_dc_Pbat_2_err = V_dc_Pbat_rif*V_dc_Pbat_rif - V_dc*V_dc;

% Controllore di tensione. Calcola la potenza da iniettare nel condensatore del bus dc
P_bat_rif = P_bat_rif_p + K_V_dc_2_Pbat_err*V_dc_Pbat_2_err + K_V_dc_2_Pbat_err_p*V_dc_2_Pbat_err_p;


% Limitazione del riferimento di potenza
if(P_bat_rif>P_bat_rif_max)
    P_bat_rif=P_bat_rif_max;
end    

if(P_bat_rif<P_bat_rif_min)
    P_bat_rif=P_bat_rif_min;
end    


% Calcolo del riferimento di corrente di batteria a partire dal riferimento di potenza
I_bat_rif = P_bat_rif/V_bat;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%                   Controllo della corrente della batteria                          % 
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Calcolo dei riferimenti per le correnti nei due rami del convertitore buck-boost
% I due riferimento sono uguali per i due rami 
I_L_rif_A=0.5*I_bat_rif;
I_L_rif_B=I_L_rif_A;

% Controllori di corrente
% Errori di corrente
I_L_err_A=I_L_rif_A-I_L_A;
I_L_err_B=I_L_rif_B-I_L_B;


% Valori estremi della tensione applicabile sulle induttanze di filtro
V_L_rif_max=V_bat;      % Corrisponde ad avere il transistor "basso" sempre chiuso
V_L_rif_min=V_bat-V_dc; % Corrisponde ad avere il transistor "alto" sempre chiuso

% Controllori di corrente. Calcolano la tensione da applicare sulle induttanze di filtro
V_L_A_rif = V_L_rif_A_p + K_I_L_err*I_L_err_A + K_I_L_err_p*I_L_err_A_p;
V_L_B_rif = V_L_rif_B_p + K_I_L_err*I_L_err_B + K_I_L_err_p*I_L_err_B_p;


% Limitazione dei riferimenti per evitare windup (il duty cycle non può essere maggiore di 1)
if(V_L_A_rif>V_L_rif_max)
    V_L_A_rif=V_L_rif_max;
end 

if(V_L_A_rif<V_L_rif_min)
    V_L_A_rif=V_L_rif_min;
end 

if(V_L_B_rif>V_L_rif_max)
    V_L_B_rif=V_L_rif_max;
end 

if(V_L_B_rif<V_L_rif_min)
    V_L_B_rif=V_L_rif_min;
end 

% Tensioni da generare all'ingresso (punto di mezzo dei due transistor) del buck-boost
V_buckboost_A = V_bat - V_L_A_rif;
V_buckboost_B = V_bat - V_L_B_rif;

% Duty cycle corrispondenti alle tensioni da generare
duty_cycle_PWM_A = V_buckboost_A / V_dc;
duty_cycle_PWM_B = V_buckboost_B / V_dc;



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%           Controllo dell'ampiezza della corrente nella bobina remota               %
%            mediante tensione generata dal convertitoee dc-ac locale                % 
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Massima tensione ac che può essere generata
V_ac_rif_max = V_dc*4/pi;

% Errore di ampiezza della corrente locale
% Questa grandezza va trasmessa via radio all'altra sezione del sistema
I_coil_loc_err=I_coil_loc_rif-I_coil_loc;

% Riferimento di tensione
V_ac_rif = K_V_coil_rif_p*V_ac_rif_p + K_V_coil_rif_pp*V_ac_rif_pp + K_I_coil_err*I_coil_rem_err + K_I_coil_err_p*I_coil_rem_err_p + K_I_coil_err_pp*I_coil_rem_err_pp;

% Limitazione del riferimento di tensione
if(V_ac_rif>V_ac_rif_max) % La prima armonica di tensione generata dal convertitore dc-ac non può essere maggiore di 4/pi * Vdc
    V_ac_rif=V_ac_rif_max;
end 

if(V_ac_rif<0)
    V_ac_rif=0;
end 

arg_arcsin=V_ac_rif/V_ac_rif_max; % argomento della funzione arcoseno

if(arg_arcsin>1)
    arg_arcsin=1;
end    

% interpolazione della funzione arcoseno
indice_argomento=arg_arcsin*100; % indice ella tabella 
indice_argomento_prec=floor(indice_argomento); % corrisponde a (int)(indice argomento) E' l'intero immediatamente inferiore all'argomento dell'arcoseno*100. 
indice_argomento_suc=indice_argomento_prec+1; % Intero immediatamente superiore all'argomento dell'arcoseno*100.
% il valore "corretto" dell'arcoseno di arg_arcsin è compreso tra i due
% elementi della tabela che si trovano nelle posizioni indice_argomento_prec e indice_argomento_suc
duty_prec=tabella_arcsin(indice_argomento_prec+1); 
duty_suc=tabella_arcsin(indice_argomento_suc+1);

% Approssimo il valore corretto mediante una interpolazione lineare
delta_indice_argomento=indice_argomento-indice_argomento_prec; 
duty=duty_prec+delta_indice_argomento*(duty_suc-duty_prec);

duty_cycle_PS_A=0.5+duty;
duty_cycle_PS_B=0.5-duty;

  

% Aggiornamento delle variabili da usare nel prossimo istante di campionamento
if(Contatore==Ritardo_PWM)
    I_L_err_A_p=I_L_err_A;
    I_L_err_B_p=I_L_err_B;
    V_L_rif_A_p=V_L_A_rif;
    V_L_rif_B_p=V_L_B_rif;
    V_dc_2_Pbat_err_p = V_dc_Pbat_2_err;
    P_bat_rif_p = P_bat_rif;
    I_coil_rem_err_pp =  I_coil_rem_err_p;
    I_coil_rem_err_p = I_coil_rem_err;
    V_ac_rif_pp = V_ac_rif_p;
    V_ac_rif_p = V_ac_rif;
else
    I_L_err_A_p=I_L_err_A;
    I_L_err_B_p=I_L_err_B;
    V_L_rif_A_p=V_L_A_rif;
    V_L_rif_B_p=V_L_B_rif;
end